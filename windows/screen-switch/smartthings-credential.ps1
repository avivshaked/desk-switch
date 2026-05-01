param(
    [ValidateSet("set", "get", "delete")]
    [string]$Action = "get",

    [string]$Target = "desk-switch-smartthings-token"
)

$ErrorActionPreference = "Stop"

$credManSource = @"
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class CredMan {
    private const int CRED_TYPE_GENERIC = 1;
    private const int CRED_PERSIST_LOCAL_MACHINE = 2;

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct CREDENTIAL {
        public UInt32 Flags;
        public UInt32 Type;
        public string TargetName;
        public string Comment;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
        public UInt32 CredentialBlobSize;
        public IntPtr CredentialBlob;
        public UInt32 Persist;
        public UInt32 AttributeCount;
        public IntPtr Attributes;
        public string TargetAlias;
        public string UserName;
    }

    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredWrite(ref CREDENTIAL credential, UInt32 flags);

    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredRead(string target, UInt32 type, UInt32 reservedFlag, out IntPtr credentialPtr);

    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredDelete(string target, UInt32 type, UInt32 flags);

    [DllImport("advapi32.dll", SetLastError = true)]
    private static extern void CredFree(IntPtr buffer);

    public static void WriteToken(string target, string token) {
        byte[] tokenBytes = Encoding.Unicode.GetBytes(token);
        IntPtr blob = Marshal.AllocHGlobal(tokenBytes.Length);
        try {
            Marshal.Copy(tokenBytes, 0, blob, tokenBytes.Length);
            CREDENTIAL credential = new CREDENTIAL();
            credential.Type = CRED_TYPE_GENERIC;
            credential.TargetName = target;
            credential.CredentialBlobSize = (UInt32)tokenBytes.Length;
            credential.CredentialBlob = blob;
            credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
            credential.UserName = Environment.UserName;

            if (!CredWrite(ref credential, 0)) {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }
        finally {
            Marshal.FreeHGlobal(blob);
        }
    }

    public static string ReadToken(string target) {
        IntPtr credentialPtr;
        if (!CredRead(target, CRED_TYPE_GENERIC, 0, out credentialPtr)) {
            throw new Win32Exception(Marshal.GetLastWin32Error());
        }

        try {
            CREDENTIAL credential = (CREDENTIAL)Marshal.PtrToStructure(credentialPtr, typeof(CREDENTIAL));
            if (credential.CredentialBlobSize == 0 || credential.CredentialBlob == IntPtr.Zero) {
                return "";
            }
            byte[] tokenBytes = new byte[credential.CredentialBlobSize];
            Marshal.Copy(credential.CredentialBlob, tokenBytes, 0, tokenBytes.Length);
            return Encoding.Unicode.GetString(tokenBytes);
        }
        finally {
            CredFree(credentialPtr);
        }
    }

    public static bool DeleteToken(string target) {
        if (CredDelete(target, CRED_TYPE_GENERIC, 0)) {
            return true;
        }

        int error = Marshal.GetLastWin32Error();
        if (error == 1168) {
            return false;
        }
        throw new Win32Exception(error);
    }
}
"@

Add-Type -TypeDefinition $credManSource

switch ($Action) {
    "set" {
        $secureToken = Read-Host "SmartThings token" -AsSecureString
        $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureToken)
        try {
            $token = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
            if ([string]::IsNullOrWhiteSpace($token)) {
                throw "Token was empty."
            }
            [CredMan]::WriteToken($Target, $token)
            Write-Host "Stored SmartThings token in Windows Credential Manager target: $Target"
        }
        finally {
            if ($bstr -ne [IntPtr]::Zero) {
                [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
            }
            Remove-Variable token -ErrorAction SilentlyContinue
        }
    }
    "get" {
        [CredMan]::ReadToken($Target)
    }
    "delete" {
        if ([CredMan]::DeleteToken($Target)) {
            Write-Host "Deleted Windows Credential Manager target: $Target"
        } else {
            Write-Host "Credential target was not present: $Target"
        }
    }
}
