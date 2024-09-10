#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <IndustryStandard/Acpi.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SimpleFileSystem.h>

// This function converts acpi table signature to acpi ami file name in CHAR16 formatt
// Ex: DSDT => DSDT.aml
CHAR16 *ConvertTableSignatureToAmlFileName(UINT32 table_signature){
    // Count the Char8 str size first
    CHAR16 *str;
    gBS->AllocatePool(EfiBootServicesCode, 8 * 2, (VOID**)&str);
    for (UINT8 i = 0; i < 4; i++) {
        str[i] = table_signature & 0xff;
        table_signature = table_signature >> 8;
    }
    str[4] = '.';
    str[5] = 'a';
    str[6] = 'm';
    str[7] = 'l';
    str[8] = 0;
    return str;
}



// Function to open a file for writing
EFI_STATUS
OpenFileForWrite(
    EFI_FILE_PROTOCOL **File,
    CHAR16 *FileName
)
{
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;

    // Locate the file system protocol
    Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&FileSystem);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate file system protocol: %r\n", Status);
        return Status;
    }

    // Open the volume (root of the filesystem)
    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open volume: %r\n", Status);
        return Status;
    }

    // Open the file for writing (create if it doesn't exist)
    Status = Root->Open(
        Root,
        File,
        FileName,
        EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open file %s: %r\n", FileName, Status);
    }

    return Status;
}

EFI_STATUS
SaveTableToAml(EFI_ACPI_SDT_HEADER *DsdtTable){
    EFI_FILE_PROTOCOL *File;
    EFI_STATUS Status;
    CHAR16 *FileName = ConvertTableSignatureToAmlFileName(DsdtTable->Signature);

    // Open the file for writing
    Status = OpenFileForWrite(&File, FileName);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open file for writing\n");
        return Status;
    }

    // Write the DSDT table to the file
    UINTN BufferSize = DsdtTable->Length;

    //Print(L"Table Signature = %s\n", FileName);
    AsciiPrint("Table Signature = ");
    for (UINT8 i = 0; i < sizeof(DsdtTable->Signature); i++)
        AsciiPrint("%c", (DsdtTable->Signature >> (8 * i)) & 0xff);
    Print(L"\n");
    Print(L"Table table size =%d\n", DsdtTable->Length);
    Print(L"Table checksum  =%d\n", DsdtTable->Checksum);

    /*for (UINT16 k = 0 ; k < 0x100; k++){
        Print(L"%02x ", ((UINT8*)DsdtTable)[k]);
        if (k != 0 && k % 0x10 == 0)
            Print(L"\n");
    }*/

    Status = File->Write(File, &BufferSize, DsdtTable);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to write DSDT table to file: %r\n", Status);
    } else {
        Print(L"DSDT table saved as %s\n", FileName);
    }
    // Close the file
    Status = File->Close(File);
    return Status;
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS                Status;
    EFI_ACPI_SDT_PROTOCOL      *AcpiSdt;
    EFI_ACPI_SDT_HEADER        *DsdtTable = NULL;
    UINTN                      TableKey;
    UINTN                      Index;
    UINT32 TableVersion = 0;


    // Locate EFI_ACPI_SDT_PROTOCOL
    Status = gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID**)&AcpiSdt);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate EFI_ACPI_SDT_PROTOCOL: %r\n", Status);
        return Status;
    }

    // Find the DSDT table
    for (Index = 0; ; Index++) {
        Print(L"Getting table %d \n", Index);
        if (AcpiSdt->GetAcpiTable == NULL){
          Print(L"AcpiSdt->GetAcpiTable is null\n");

        }
        Status = AcpiSdt->GetAcpiTable(Index, &DsdtTable, &TableVersion, &TableKey);
        if (DsdtTable == NULL){
          Print(L"table %d ptr is null\n", Index);
          continue;
        }

        SaveTableToAml(DsdtTable);

        if (EFI_ERROR(Status)) {
            Print(L"No more ACPI tables found\n");
            return EFI_SUCCESS;
        }

    }
 

    // Save the DSDT table to a file
    return EFI_SUCCESS;
}
