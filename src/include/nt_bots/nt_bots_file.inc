#if !defined(_NT_BOTS_FILE_INC_)
#define _NT_BOTS_FILE_INC_

// Reads binary data from a file, and interprets it as contiguous uint32
// array. Returns number of elements read, or -1 on error.
int ReadFileUint32(File file, int[] items, int num_items)
{
    if (num_items < 1) {
        return 0;
    }
    
#define SIZEOF_UINT32_BYTES 4
#define NUM_UI32_BYTES num_items * SIZEOF_UINT32_BYTES
    decl String:bytesBuffer[NUM_UI32_BYTES];
    for (int byteIt = 0; byteIt < NUM_UI32_BYTES; ++byteIt) {
        decl String:oneByteBuffer[1];
        if (ReadFileString(file, oneByteBuffer, 1, 1) != 1)
        {
            ThrowError("Read failed at file position %i (is end of file: %s)",
                FilePosition(file),
                (IsEndOfFile(file) ? "yes" : "no"));
            return -1;
        }
        bytesBuffer[byteIt] = oneByteBuffer[0];
    }
    
    PrintToServer("UINT32 %u as hex: %x (binary %b). Would be %d signed.",
        bytesBuffer, bytesBuffer, bytesBuffer, bytesBuffer);
    
    PrintToServer("Bytes:%b, %b, %b, %b",
        bytesBuffer[0], bytesBuffer[1], bytesBuffer[2], bytesBuffer[3]);
    
#define UINT32_STR_REPRESENTATION_MAXLEN 10
    decl String:i32String[UINT32_STR_REPRESENTATION_MAXLEN + 1];
    
    int i;
    for (i = 0; i < num_items; ++i) {
        PrintToServer("Num: i32: %i ui32: %u hex: %x",
            view_as<int>(bytesBuffer[i * SIZEOF_UINT32_BYTES]),
            bytesBuffer[i * SIZEOF_UINT32_BYTES],
            bytesBuffer[i * SIZEOF_UINT32_BYTES]);
        
        // Raw hex to unsigned 32int to signed SourceMod int32
        if (Format(i32String, UINT32_STR_REPRESENTATION_MAXLEN + 1,
            "%u", bytesBuffer[i * SIZEOF_UINT32_BYTES]) < 1)
        {
            LogError("Float format failed");
            return -1;
        }
        items[i] = StringToInt(i32String);
        PrintToServer("String res: %s as int32: %i",
            i32String, items[i]);
    }
    return i;
}

// Reads binary data from a file, and interprets it as contiguous float array.
// Returns number of elements read, or -1 on error.
int ReadFileFloat(File file, float[] items, int num_items)
{
    if (num_items < 1) {
        return 0;
    }
    
#define SIZEOF_FLOAT_BYTES 4
#define NUM_FLOAT_BYTES num_items * SIZEOF_FLOAT_BYTES
    decl String:bytesBuffer[NUM_FLOAT_BYTES];
    for (int byteIt = 0; byteIt < NUM_FLOAT_BYTES; ++byteIt) {
        decl String:oneByteBuffer[1];
        if (ReadFileString(file, oneByteBuffer, 1, 1) != 1)
        {
            ThrowError("Read failed at file position %i (is end of file: %s)",
                FilePosition(file),
                (IsEndOfFile(file) ? "yes" : "no"));
            return -1;
        }
        bytesBuffer[byteIt] = oneByteBuffer[0];
    }
    
    // Max characters for storing significant figures. Increase if you need more.
#define FLOAT_STR_REPRESENTATION_MAXLEN 31
    decl String:floatString[FLOAT_STR_REPRESENTATION_MAXLEN + 1];
    
    int i;
    for (i = 0; i < num_items; ++i) {
        // SourcePawn (1.10.0.6478) gets confused if we do a raw byte cast
        // on the array pointer, so routing the reinterpret via string format.
        if (Format(floatString, FLOAT_STR_REPRESENTATION_MAXLEN + 1,
            "%f", view_as<float>(bytesBuffer[i * SIZEOF_FLOAT_BYTES])) < 1)
        {
            LogError("Float format failed");
            return -1;
        }
        items[i] = StringToFloat(floatString);
        PrintToServer("String res: %s as float: %f",
            floatString, items[i]);
    }
    return i;
}

#endif // _NT_BOTS_FILE_INC_