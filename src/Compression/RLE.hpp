
#pragma once

//===========================================================================
struct NRLE
{
// Invariant: for signed VT, encoded values have all high unit-aligned 1s stripped.
// Initializing Value to -1 restores proper two's-complement sign extension.
// 
// UnitsNum: 00,01,10,1100,1101,1110,111100    // Prefix encoding (Unit counting)
//           0  1  2  3    4    5    6
//           1  2  3  4    5    6    7  // units
// Values are BigEndian
//
// ULen: Size of bit unit (VarInt prefix will count in those)
//
template<typename VT, uint ULen=4> static int ReadBitVarInt(auto* bs, VT& val)   // TODO: Make sure it works with BigNum class
{
// static_assert(ULen > 0, "ULen must be > 0");
 using RT = MakeUnsignedT<VT>;
 SCVR uint BLenVT = sizeof(VT)*8;
 SCVR bool SignV  = IsSignedV<VT>;  
 uint32 UCtr = 0;   // 0 = 1 unit
 for(;;)    // TODO: Optimize (Read uint64 and seek back?)  // Do benchmark
  {
   uint32 tmp;
   usize res = bs->Read(&tmp, 2);
   if(STRM::IsError(res))return res;
   UCtr += tmp; 
   if(tmp != 3)break;  // This one is last
  }
 uint32 UBLen = ++UCtr * ULen; 
 if(UBLen > BLenVT)return PX::ERANGE;  // Skip high bits which won't fit into VT type?  // The value will be corrupted anyway
 RT   Bits = 0;
 usize res = bs->Read(&Bits, UBLen);     // Read a piece of the bitstream
 if(STRM::IsError(res))return res;
 if constexpr (SignV)
  {
   if(Bits >> (UBLen-1))Bits |= (RT(-1) << UBLen);   // Extend negative sign bit
  }
 val = VT(Bits);
 return 0;
}
//---------------------------------------------------------------------------
// VLen: Max number of bits to take from 'val'
// ULen: Size of bit unit (VarInt prefix will count in those)
//
template<typename VT, uint ULen=4> static int WriteBitVarInt(auto* bs, VT val)
{
 using ST = MakeUnsignedT<VT>;
 SCVR  uint32 VMax   = sizeof(VT)*8;
 SCVR  bool   SignV  = IsSignedV<VT>;
 const bool   NegVal = SignV && (val < 0);   // Need one extra bit for sign
 ST SrcV = ST(val);
 const uint32 BLenV = (VMax - clz((NegVal?-SrcV:SrcV))) + SignV;       // Number of bits to preserve    // ?????????? Invert negative ???????????????????
 const uint32 NumU  = (BLenV / ULen) + bool(BLenV % ULen);  // Number of whole units to store valuable bits   // Should be well optimized for Pow2 unit sizes
 const uint32 BitsN = NumU * ULen;
 const uint32 uznm  = NumU - 1;
 const uint32 gcnt  = uznm / 3;  // Number of groups ('11' markers)
 const uint32 ucnt  = uznm % 3;
 const uint32 gbits = gcnt * 2;
 const uint32 cbits = gbits + 2;
 const uint64 vctr  = (uint64(-1) << gbits) | ucnt;   // uint64 is max counter size (BigInt support?) // TODO: static assert? Too much to calculate at compile time - just use SLEB128 for >= int128 values 
 
 usize res = bs->Write(vctr, cbits);     // Write the VarInt unit counter
 if(STRM::IsFail(res))return res;

 res = bs->Write(SrcV, BitsN);     // Write piece of the bitstream 
 if(STRM::IsFail(res))return res;
 return 0;
}
//---------------------------------------------------------------------------
// TS:   Byte stream for the bit stream
// VLen: Value size in bits
// ULen: Counter unit size (may help to tune compression for frequent large/small ranges)
//
// No prefixes. Every duplicated value have a counter. 
// And every chain of single values have its counter too (Negative)
//
template<typename TS, uint VLen=8, uint ULen=4> class CRdBitStrmRLE
{
 using BS = STRM::CBitStreamMSB<TS>;     // Using MSB - faster and easier to read
 using VT = TypeForSizeT<VLen/8>;  // Value unit
 using CT = uint32;      // Should be enough?
 CT Counter;
 VT Value;
 BS BitStrm;

//-------------------------------------------
usize Read(VT* Val)
{
 
 return 0;
}
//-------------------------------------------
};
//---------------------------------------------------------------------------
template<typename TS, uint VLen=8, uint ULen=4> class CWrBitStrmRLE
{
 using BS = STRM::CBitStreamMSB<TS>;     // Using MSB - faster and easier to read
 using VT = TypeForSizeT<VLen/8>;  // Value unit
 using CT = uint32;      // Should be enough?
 CT Counter;
 VT Value;
 BS BitStrm;

//-------------------------------------------
usize Write(VT Val)
{
 return 0;
}
//-------------------------------------------
};
//---------------------------------------------------------------------------
   /*
template<typename VT, int UCtrLen, int UValLen = 0>
class CRLEBitStreamBase {
public:
    static_assert(UCtrLen > 0 && UCtrLen <= 32, "UCtrLen must be between 1 and 32");
    static_assert(UValLen >= 0 && UValLen <= 32, "UValLen must be between 0 and 32");
    
    using ValueType = VT;
    static constexpr int CounterUnitLen = UCtrLen;
    static constexpr int ValueUnitLen = UValLen;
    static constexpr bool IsValueVarInt = (UValLen > 0);
    static constexpr bool IsValueSigned = std::is_signed<VT>::value;
    
protected:
    // Encode VarInt with the pattern: 00, 01, 10, 1100, 1101, 1110, 111100, ...
    // Returns number of bits written
    template<typename TS>
    static uint WriteVarInt(CBitStream<TS>& bs, uint64_t value, int unitLen) {
        // Calculate number of units needed
        int numUnits = CalculateUnits(value, unitLen);
        
        // Write prefix bits (numUnits-1 ones, then a zero)
        uint bitsWritten = 0;
        for (int i = 0; i < numUnits - 1; ++i) {
            bs.Write<usize>(1, 1);
            bitsWritten++;
        }
        bs.Write<usize>(0, 1);
        bitsWritten++;
        
        // Write the value in the appropriate number of units
        int valueBits = numUnits * unitLen;
        if (valueBits <= 64) {
            bs.Write<uint64_t>(value, valueBits);
        } else {
            // Handle large values by splitting into chunks
            for (int i = 0; i < numUnits; ++i) {
                uint64_t chunk = (value >> (i * unitLen)) & ((1ULL << unitLen) - 1);
                bs.Write<uint64_t>(chunk, unitLen);
            }
        }
        bitsWritten += valueBits;
        
        return bitsWritten;
    }
    
    // Decode VarInt
    // Returns the decoded value
    template<typename TS>
    static uint64_t ReadVarInt(CBitStream<TS>& bs, int unitLen) {
        // Read prefix to determine number of units
        int numUnits = 1;
        usize bit;
        while (bs.Read(&bit, 1) && bit == 1) {
            ++numUnits;
        }
        
        // Read the value
        uint64_t value = 0;
        int valueBits = numUnits * unitLen;
        
        if (valueBits <= 64) {
            bs.Read(&value, valueBits);
        } else {
            // Handle large values by reading chunks
            for (int i = 0; i < numUnits; ++i) {
                uint64_t chunk;
                bs.Read(&chunk, unitLen);
                value |= (chunk << (i * unitLen));
            }
        }
        
        return value;
    }
    
    // Calculate number of units needed for a value
    static int CalculateUnits(uint64_t value, int unitLen) {
        if (value == 0) return 1;
        
        int units = 1;
        uint64_t maxVal = (1ULL << unitLen) - 1;
        
        while (value > maxVal) {
            ++units;
            if (units * unitLen >= 64) break;  // Prevent overflow
            maxVal = (maxVal << unitLen) | ((1ULL << unitLen) - 1);
        }
        
        return units;
    }
    
    // Encode signed value using zigzag encoding
    static uint64_t EncodeSignedValue(int64_t value) {
        return (value << 1) ^ (value >> 63);
    }
    
    // Decode signed value from zigzag encoding
    static int64_t DecodeSignedValue(uint64_t encoded) {
        return (encoded >> 1) ^ -(int64_t)(encoded & 1);
    }
    
    // Write a single value
    template<typename TS>
    static void WriteValue(CBitStream<TS>& bs, ValueType value) {
        if constexpr (IsValueVarInt) {
            // Write value as VarInt
            if constexpr (IsValueSigned) {
                uint64_t encoded = EncodeSignedValue(static_cast<int64_t>(value));
                WriteVarInt(bs, encoded, ValueUnitLen);
            } else {
                WriteVarInt(bs, static_cast<uint64_t>(value), ValueUnitLen);
            }
        } else {
            // Write fixed-size value
            constexpr int valueBits = sizeof(ValueType) * 8;
            if constexpr (IsValueSigned) {
                uint64_t encoded = EncodeSignedValue(static_cast<int64_t>(value));
                bs.Write<uint64_t>(encoded, valueBits);
            } else {
                bs.Write<uint64_t>(static_cast<uint64_t>(value), valueBits);
            }
        }
    }
    
    // Read a single value
    template<typename TS>
    static ValueType ReadValue(CBitStream<TS>& bs) {
        if constexpr (IsValueVarInt) {
            // Read VarInt value
            uint64_t encoded = ReadVarInt(bs, ValueUnitLen);
            
            if constexpr (IsValueSigned) {
                return static_cast<ValueType>(DecodeSignedValue(encoded));
            } else {
                return static_cast<ValueType>(encoded);
            }
        } else {
            // Read fixed-size value
            constexpr int valueBits = sizeof(ValueType) * 8;
            uint64_t encoded;
            bs.Read(&encoded, valueBits);
            
            if constexpr (IsValueSigned) {
                return static_cast<ValueType>(DecodeSignedValue(encoded));
            } else {
                return static_cast<ValueType>(encoded);
            }
        }
    }
};   */



};
//===========================================================================
