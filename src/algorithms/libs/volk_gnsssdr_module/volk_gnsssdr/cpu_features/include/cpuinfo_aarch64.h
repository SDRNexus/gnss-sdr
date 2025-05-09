// SPDX-FileCopyrightText: 2017 Google LLC
// SPDX-License-Identifier: Apache-2.0

////////////////////////////////////////////////////////////////////////////////
// A note on Windows AArch64 implementation
////////////////////////////////////////////////////////////////////////////////

// Getting cpu info via EL1 system registers is not possible, so we delegate it
// to the Windows API (i.e., IsProcessorFeaturePresent and GetNativeSystemInfo).
// The `implementer`, `variant` and `part` fields of the `Aarch64Info` struct
// are not used, so they are set to 0. To get `revision` we use
// `wProcessorRevision` from `SYSTEM_INFO`.
//
// Cryptographic Extension:
// -----------------------------------------------------------------------------
// According to documentation Arm Architecture Reference Manual for
// A-profile architecture. A2.3 The Armv8 Cryptographic Extension. The Armv8.0
// Cryptographic Extension provides instructions for the acceleration of
// encryption and decryption, and includes the following features: FEAT_AES,
// FEAT_PMULL, FEAT_SHA1, FEAT_SHA256.
// see: https://developer.arm.com/documentation/ddi0487/latest
//
// We use `PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE` to detect all Armv8.0 crypto
// features. This value reports all features or nothing, so even if you only
// have support FEAT_AES and FEAT_PMULL, it will still return false.
//
// From Armv8.2, an implementation of the Armv8.0 Cryptographic Extension can
// include either or both of:
//
// • The AES functionality, including support for multiplication of 64-bit
//   polynomials. The ID_AA64ISAR0_EL1.AES field indicates whether this
//   functionality is supported.
// • The SHA1 and SHA2-256 functionality. The ID_AA64ISAR0_EL1.{SHA2, SHA1}
//   fields indicate whether this functionality is supported.
//
// ID_AA64ISAR0_EL1.AES, bits [7:4]:
// Indicates support for AES instructions in AArch64 state. Defined values are:
// - 0b0000 No AES instructions implemented.
// - 0b0001 AESE, AESD, AESMC, and AESIMC instructions implemented.
// - 0b0010 As for 0b0001, plus PMULL/PMULL2 instructions operating on 64-bit
//   data quantities.
//
// FEAT_AES implements the functionality identified by the value 0b0001.
// FEAT_PMULL implements the functionality identified by the value 0b0010.
// From Armv8, the permitted values are 0b0000 and 0b0010.
//
// ID_AA64ISAR0_EL1.SHA1, bits [11:8]:
// Indicates support for SHA1 instructions in AArch64 state. Defined values are:
// - 0b0000 No SHA1 instructions implemented.
// - 0b0001 SHA1C, SHA1P, SHA1M, SHA1H, SHA1SU0, and SHA1SU1 instructions
//   implemented.
//
// FEAT_SHA1 implements the functionality identified by the value 0b0001.
// From Armv8, the permitted values are 0b0000 and 0b0001.
// If the value of ID_AA64ISAR0_EL1.SHA2 is 0b0000, this field must have the
// value 0b0000.
//
// ID_AA64ISAR0_EL1.SHA2, bits [15:12]:
// Indicates support for SHA2 instructions in AArch64 state. Defined values are:
// - 0b0000 No SHA2 instructions implemented.
// - 0b0001 Implements instructions: SHA256H, SHA256H2, SHA256SU0, and
//   SHA256SU1.
// - 0b0010 Implements instructions:
//          • SHA256H, SHA256H2, SHA256SU0, and SHA256SU1.
//          • SHA512H, SHA512H2, SHA512SU0, and SHA512SU1.
//
// FEAT_SHA256 implements the functionality identified by the value 0b0001.
// FEAT_SHA512 implements the functionality identified by the value 0b0010.
//
// In Armv8, the permitted values are 0b0000 and 0b0001.
// From Armv8.2, the permitted values are 0b0000, 0b0001, and 0b0010.
//
// If the value of ID_AA64ISAR0_EL1.SHA1 is 0b0000, this field must have the
// value 0b0000.
//
// If the value of this field is 0b0010, ID_AA64ISAR0_EL1.SHA3
// must have the value 0b0001.
//
// Other cryptographic features that we cannot detect such as sha512, sha3, sm3,
// sm4, sveaes, svepmull, svesha3, svesm4 we set to 0.
//
// FP/SIMD:
// -----------------------------------------------------------------------------
// FP/SIMD must be implemented on all Armv8.0 implementations, but
// implementations targeting specialized markets may support the following
// combinations:
//
// • No NEON or floating-point.
// • Full floating-point and SIMD support with exception trapping.
// • Full floating-point and SIMD support without exception trapping.
//
// ref:
// https://developer.arm.com/documentation/den0024/a/AArch64-Floating-point-and-NEON
//
// So, we use `PF_ARM_VFP_32_REGISTERS_AVAILABLE`,
// `PF_ARM_NEON_INSTRUCTIONS_AVAILABLE` to detect `asimd` and `fp`

#ifndef CPU_FEATURES_INCLUDE_CPUINFO_AARCH64_H_
#define CPU_FEATURES_INCLUDE_CPUINFO_AARCH64_H_

#include "cpu_features_cache_info.h"
#include "cpu_features_macros.h"

CPU_FEATURES_START_CPP_NAMESPACE

typedef struct
{
    int fp : 1;          // Floating-point.
    int asimd : 1;       // Advanced SIMD.
    int evtstrm : 1;     // Generic timer generated events.
    int aes : 1;         // Hardware-accelerated Advanced Encryption Standard.
    int pmull : 1;       // Polynomial multiply long.
    int sha1 : 1;        // Hardware-accelerated SHA1.
    int sha2 : 1;        // Hardware-accelerated SHA2-256.
    int crc32 : 1;       // Hardware-accelerated CRC-32.
    int atomics : 1;     // Armv8.1 atomic instructions.
    int fphp : 1;        // Half-precision floating point support.
    int asimdhp : 1;     // Advanced SIMD half-precision support.
    int cpuid : 1;       // Access to certain ID registers.
    int asimdrdm : 1;    // Rounding Double Multiply Accumulate/Subtract.
    int jscvt : 1;       // Support for JavaScript conversion.
    int fcma : 1;        // Floating point complex numbers.
    int lrcpc : 1;       // Support for weaker release consistency.
    int dcpop : 1;       // Data persistence writeback.
    int sha3 : 1;        // Hardware-accelerated SHA3.
    int sm3 : 1;         // Hardware-accelerated SM3.
    int sm4 : 1;         // Hardware-accelerated SM4.
    int asimddp : 1;     // Dot product instruction.
    int sha512 : 1;      // Hardware-accelerated SHA512.
    int sve : 1;         // Scalable Vector Extension.
    int asimdfhm : 1;    // Additional half-precision instructions.
    int dit : 1;         // Data independent timing.
    int uscat : 1;       // Unaligned atomics support.
    int ilrcpc : 1;      // Additional support for weaker release consistency.
    int flagm : 1;       // Flag manipulation instructions.
    int ssbs : 1;        // Speculative Store Bypass Safe PSTATE bit.
    int sb : 1;          // Speculation barrier.
    int paca : 1;        // Address authentication.
    int pacg : 1;        // Generic authentication.
    int dcpodp : 1;      // Data cache clean to point of persistence.
    int sve2 : 1;        // Scalable Vector Extension (version 2).
    int sveaes : 1;      // SVE AES instructions.
    int svepmull : 1;    // SVE polynomial multiply long instructions.
    int svebitperm : 1;  // SVE bit permute instructions.
    int svesha3 : 1;     // SVE SHA3 instructions.
    int svesm4 : 1;      // SVE SM4 instructions.
    int flagm2 : 1;      // Additional flag manipulation instructions.
    int frint : 1;       // Floating point to integer rounding.
    int svei8mm : 1;     // SVE Int8 matrix multiplication instructions.
    int svef32mm : 1;    // SVE FP32 matrix multiplication instruction.
    int svef64mm : 1;    // SVE FP64 matrix multiplication instructions.
    int svebf16 : 1;     // SVE BFloat16 instructions.
    int i8mm : 1;        // Int8 matrix multiplication instructions.
    int bf16 : 1;        // BFloat16 instructions.
    int dgh : 1;         // Data Gathering Hint instruction.
    int rng : 1;         // True random number generator support.
    int bti : 1;         // Branch target identification.
    int mte : 1;         // Memory tagging extension.
    int ecv : 1;         // Enhanced counter virtualization.
    int afp : 1;         // Alternate floating-point behaviour.
    int rpres : 1;       // 12-bit reciprocal (square root) estimate precision.
    int mte3 : 1;        // MTE asymmetric fault handling.
    int sme : 1;         // Scalable Matrix Extension.
    int smei16i64 : 1;   // 16-bit to 64-bit integer widening outer product.
    int smef64f64 : 1;   // FP64 to FP64 outer product.
    int smei8i32 : 1;    // 8-bit to 32-bit integer widening outer product.
    int smef16f32 : 1;   // FP16 to FP32 outer product.
    int smeb16f32 : 1;   // BFloat16 to FP32 outper product.
    int smef32f32 : 1;   // FP32 to FP32 outer product.
    int smefa64 : 1;     // Full A64 support for SME in streaming mode.
    int wfxt : 1;        // WFE and WFI with timeout.
    int ebf16 : 1;       // Extended BFloat16 instructions.
    int sveebf16 : 1;    // SVE BFloat16 instructions.
    int cssc : 1;        // Common short sequence compression instructions.
    int rprfm : 1;       // Range Prefetch Memory hint instruction.
    int sve2p1 : 1;      // Scalable Vector Extension (version 2.1).
    int sme2 : 1;        // Scalable Matrix Extension (version 2).
    int sme2p1 : 1;      // Scalable Matrix Extension (version 2.1).
    int smei16i32 : 1;   // 16-bit to 64-bit integer widening outer product.
    int smebi32i32 : 1;  // 1-bit binary to 32-bit integer outer product.
    int smeb16b16 : 1;   // SME2.1 BFloat16 instructions.
    int smef16f16 : 1;   // FP16 to FP16 outer product.
    int mops : 1;        // Standardized memory operations.
    int hbc : 1;         // Hinted conditional branches.
    int sveb16b16 : 1;   // Non-widening BFloat16 to BFloat16 arithmetic for SVE2
                         // and SME2.
    int lrcpc3 : 1;      // Load-Acquire RCpc instructions version 3.
    int lse128 : 1;      // 128-bit Atomics.
    int fpmr : 1;        // Floating-point Mode Register.
    int lut : 1;         // Lookup table instructions with 2-bit and 4-bit indices.
    int faminmax : 1;    // Maximum and minimum absolute value instructions.
    int f8cvt : 1;       // FP scaling instructions and FP8 convert instructions.
    int f8fma : 1;       // FP8 to single-precision and half-precision
                         // multiply-accumulate instructions.
    int f8dp4 : 1;       // FP8 to single-precision 4-way dot product FDOT (4-way)
                         // instructions.
    int f8dp2 : 1;       // FP8 to half-precision 2-way dot product FDOT (2-way)
                         // instructions.
    int f8e4m3 : 1;      // Arm FP8 E4M3 format.
    int f8e5m2 : 1;      // Arm FP8 E5M2 format.
    int smelutv2 : 1;    // SME2 lookup table LUTI4 and MOVT instructions.
    int smef8f16 : 1;    // SME2 F8F16 instructions.
    int smef8f32 : 1;    // SME2 F8F32 instructions.
    int smesf8fma : 1;   // SVE2 FP8 to single-precision and half-precision
                         // multiply-accumulate instructions.
    int smesf8dp4 : 1;   // SVE2 FP8 to single-precision 4-way dot product FDOT
                         // (4-way) instructions.
    int smesf8dp2 : 1;   // SVE2 FP8 to half-precision 2-way dot product FDOT
                         // (2-way) instructions.

    // Make sure to update Aarch64FeaturesEnum below if you add a field here.
} Aarch64Features;

typedef struct
{
    Aarch64Features features;
    int implementer;  // We set 0 for Windows.
    int variant;      // We set 0 for Windows.
    int part;         // We set 0 for Windows.
    int revision;     // We use GetNativeSystemInfo to get processor revision for
                      // Windows.
} Aarch64Info;

Aarch64Info GetAarch64Info(void);

////////////////////////////////////////////////////////////////////////////////
// Introspection functions

typedef enum
{
    AARCH64_FP,
    AARCH64_ASIMD,
    AARCH64_EVTSTRM,
    AARCH64_AES,
    AARCH64_PMULL,
    AARCH64_SHA1,
    AARCH64_SHA2,
    AARCH64_CRC32,
    AARCH64_ATOMICS,
    AARCH64_FPHP,
    AARCH64_ASIMDHP,
    AARCH64_CPUID,
    AARCH64_ASIMDRDM,
    AARCH64_JSCVT,
    AARCH64_FCMA,
    AARCH64_LRCPC,
    AARCH64_DCPOP,
    AARCH64_SHA3,
    AARCH64_SM3,
    AARCH64_SM4,
    AARCH64_ASIMDDP,
    AARCH64_SHA512,
    AARCH64_SVE,
    AARCH64_ASIMDFHM,
    AARCH64_DIT,
    AARCH64_USCAT,
    AARCH64_ILRCPC,
    AARCH64_FLAGM,
    AARCH64_SSBS,
    AARCH64_SB,
    AARCH64_PACA,
    AARCH64_PACG,
    AARCH64_DCPODP,
    AARCH64_SVE2,
    AARCH64_SVEAES,
    AARCH64_SVEPMULL,
    AARCH64_SVEBITPERM,
    AARCH64_SVESHA3,
    AARCH64_SVESM4,
    AARCH64_FLAGM2,
    AARCH64_FRINT,
    AARCH64_SVEI8MM,
    AARCH64_SVEF32MM,
    AARCH64_SVEF64MM,
    AARCH64_SVEBF16,
    AARCH64_I8MM,
    AARCH64_BF16,
    AARCH64_DGH,
    AARCH64_RNG,
    AARCH64_BTI,
    AARCH64_MTE,
    AARCH64_ECV,
    AARCH64_AFP,
    AARCH64_RPRES,
    AARCH64_MTE3,
    AARCH64_SME,
    AARCH64_SME_I16I64,
    AARCH64_SME_F64F64,
    AARCH64_SME_I8I32,
    AARCH64_SME_F16F32,
    AARCH64_SME_B16F32,
    AARCH64_SME_F32F32,
    AARCH64_SME_FA64,
    AARCH64_WFXT,
    AARCH64_EBF16,
    AARCH64_SVE_EBF16,
    AARCH64_CSSC,
    AARCH64_RPRFM,
    AARCH64_SVE2P1,
    AARCH64_SME2,
    AARCH64_SME2P1,
    AARCH64_SME_I16I32,
    AARCH64_SME_BI32I32,
    AARCH64_SME_B16B16,
    AARCH64_SME_F16F16,
    AARCH64_MOPS,
    AARCH64_HBC,
    AARCH64_SVE_B16B16,
    AARCH64_LRCPC3,
    AARCH64_LSE128,
    AARCH64_FPMR,
    AARCH64_LUT,
    AARCH64_FAMINMAX,
    AARCH64_F8CVT,
    AARCH64_F8FMA,
    AARCH64_F8DP4,
    AARCH64_F8DP2,
    AARCH64_F8E4M3,
    AARCH64_F8E5M2,
    AARCH64_SME_LUTV2,
    AARCH64_SME_F8F16,
    AARCH64_SME_F8F32,
    AARCH64_SME_SF8FMA,
    AARCH64_SME_SF8DP4,
    AARCH64_SME_SF8DP2,
    AARCH64_LAST_,
} Aarch64FeaturesEnum;

int GetAarch64FeaturesEnumValue(const Aarch64Features* features,
    Aarch64FeaturesEnum value);

const char* GetAarch64FeaturesEnumName(Aarch64FeaturesEnum);

CPU_FEATURES_END_CPP_NAMESPACE

#if !defined(CPU_FEATURES_ARCH_AARCH64)
#error "Including cpuinfo_aarch64.h from a non-aarch64 target."
#endif

#endif  // CPU_FEATURES_INCLUDE_CPUINFO_AARCH64_H_