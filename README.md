# OfficePasswordCheckerCPP

Verify if the Office (OOXML) file is encrypted by RMS/Password

Following code verifies if the Office document is encrypted by RMS/Password only for OOXML files.  
This code is built based on the sample code in the following document.

[EnumAll Sample](https://docs.microsoft.com/ja-jp/windows/desktop/Stg/enumall-sample)

## Description

Checking if the following names are contained in enumerated Storage/Stream in the document.

- RMS Encrypted: DRMEncryptedDataSpace
- Password Encrypted: StrongEncryptionTransform/EncryptionInfo

You can use IsRMSEncrypted/IsPasswordEncrypted functions but they just check the existance of these names.  
Practically, these names cannot be the last member of the vector which contains the names.