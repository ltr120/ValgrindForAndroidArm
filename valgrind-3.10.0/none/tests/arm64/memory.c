
/* To compile:
   aarch64-linux-gnu-gcc -Wall -g -O0 -o memory none/tests/arm64/memory.c
*/

#include <stdio.h>
#include <malloc.h>  // memalign
#include <string.h>  // memset
#include <assert.h>

typedef  unsigned char           UChar;
typedef  unsigned short int      UShort;
typedef  unsigned int            UInt;
typedef  signed int              Int;
typedef  unsigned char           UChar;
typedef  signed long long int    Long;
typedef  unsigned long long int  ULong;

typedef  unsigned char           Bool;
#define False ((Bool)0)
#define True  ((Bool)1)

__attribute__((noinline))
static void* memalign16(size_t szB)
{
   void* x;
   x = memalign(16, szB);
   assert(x);
   assert(0 == ((16-1) & (unsigned long)x));
   return x;
}

static inline UChar randUChar ( void )
{
   static UInt seed = 80021;
   seed = 1103515245 * seed + 12345;
   return (seed >> 17) & 0xFF;
}

static ULong randULong ( void )
{
   Int i;
   ULong r = 0;
   for (i = 0; i < 8; i++) {
      r = (r << 8) | (ULong)(0xFF & randUChar());
   }
   return r;
}


// Same as TESTINST2 except it doesn't print the RN value, since
// that may differ between runs (it's a stack address).  Also,
// claim it trashes x28 so that can be used as scratch if needed.
#define TESTINST2_hide2(instruction, RNval, RD, RN, carryin) \
{ \
   ULong out; \
   ULong nzcv_out; \
   ULong nzcv_in = (carryin ? (1<<29) : 0); \
   __asm__ __volatile__( \
      "msr nzcv,%3;" \
      "mov " #RN ",%2;" \
      instruction ";" \
      "mov %0," #RD ";" \
      "mrs %1,nzcv;" \
      : "=&r" (out), "=&r" (nzcv_out) \
      : "r" (RNval), "r" (nzcv_in) \
      : #RD, #RN, "cc", "memory", "x28"  \
   ); \
   printf("%s :: rd %016llx rn (hidden), " \
          "cin %d, nzcv %08llx %c%c%c%c\n",       \
      instruction, out, \
      carryin ? 1 : 0, \
      nzcv_out & 0xffff0000, \
      ((1<<31) & nzcv_out) ? 'N' : ' ', \
      ((1<<30) & nzcv_out) ? 'Z' : ' ', \
      ((1<<29) & nzcv_out) ? 'C' : ' ', \
      ((1<<28) & nzcv_out) ? 'V' : ' ' \
      ); \
}

#define TESTINST3_hide2and3(instruction, RMval, RNval, RD, RM, RN, carryin) \
{ \
   ULong out; \
   ULong nzcv_out; \
   ULong nzcv_in = (carryin ? (1<<29) : 0); \
   __asm__ __volatile__( \
      "msr nzcv,%4;" \
      "mov " #RM ",%2;" \
      "mov " #RN ",%3;" \
      instruction ";" \
      "mov %0," #RD ";" \
      "mrs %1,nzcv;" \
      : "=&r" (out), "=&r" (nzcv_out) \
      : "r" (RMval), "r" (RNval), "r" (nzcv_in) \
      : #RD, #RM, #RN, "cc", "memory" \
   ); \
   printf("%s :: rd %016llx rm (hidden), rn (hidden), " \
          "cin %d, nzcv %08llx %c%c%c%c\n",       \
      instruction, out, \
      carryin ? 1 : 0, \
      nzcv_out & 0xffff0000, \
      ((1<<31) & nzcv_out) ? 'N' : ' ', \
      ((1<<30) & nzcv_out) ? 'Z' : ' ', \
      ((1<<29) & nzcv_out) ? 'C' : ' ', \
      ((1<<28) & nzcv_out) ? 'V' : ' ' \
      ); \
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

static __attribute((noinline)) void test_memory ( void )
{
printf("Integer loads\n");

unsigned char area[512];

#define RESET \
   do { int i; for (i = 0; i < sizeof(area); i++) \
          area[i] = i | 0x80; \
   } while (0)

#define AREA_MID (((ULong)(&area[(sizeof(area)/2)-1])) & (~(ULong)0xF))

RESET;

////////////////////////////////////////////////////////////////
printf("LDR,STR (immediate, uimm12) (STR cases are MISSING)");
TESTINST2_hide2("ldr  x21, [x22, #24]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldr  w21, [x22, #20]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrh w21, [x22, #44]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrb w21, [x22, #56]", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("LDUR,STUR (immediate, simm9) (STR cases and wb check are MISSING)\n");
TESTINST2_hide2("ldr x21, [x22], #-24", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldr x21, [x22, #-40]!", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldr x21, [x22, #-48]", AREA_MID, x21,x22,0);
printf("LDUR,STUR (immediate, simm9): STR cases are MISSING");

////////////////////////////////////////////////////////////////
// TESTINST2_hide2 allows use of x28 as scratch
printf("LDP,STP (immediate, simm7) (STR cases and wb check is MISSING)\n");

TESTINST2_hide2("ldp x21, x28, [x22], #-24 ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp x21, x28, [x22], #-24 ; eor x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp x21, x28, [x22, #-40]! ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp x21, x28, [x22, #-40]! ; eor x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp x21, x28, [x22, #-40] ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp x21, x28, [x22, #-40] ; eor x21,x21,x28", AREA_MID, x21,x22,0);

TESTINST2_hide2("ldp w21, w28, [x22], #-24 ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp w21, w28, [x22], #-24 ; eor x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp w21, w28, [x22, #-40]! ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp w21, w28, [x22, #-40]! ; eor x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp w21, w28, [x22, #-40] ; add x21,x21,x28", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldp w21, w28, [x22, #-40] ; eor x21,x21,x28", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
// This is a bit tricky.  We load the value from just before and
// just after the actual instruction.  Because TESTINSN2_hide2 
// generates two fixed insns either side of the test insn, these 
// should be constant and hence "safe" to check.

printf("LDR (literal, int reg)\n");
TESTINST2_hide2("xyzzy00: ldr  x21, xyzzy00 - 8", AREA_MID, x21,x22,0);
TESTINST2_hide2("xyzzy01: ldr  x21, xyzzy01 + 0", AREA_MID, x21,x22,0);
TESTINST2_hide2("xyzzy02: ldr  x21, xyzzy02 + 8", AREA_MID, x21,x22,0);

TESTINST2_hide2("xyzzy03: ldr  x21, xyzzy03 - 4", AREA_MID, x21,x22,0);
TESTINST2_hide2("xyzzy04: ldr  x21, xyzzy04 + 0", AREA_MID, x21,x22,0);
TESTINST2_hide2("xyzzy05: ldr  x21, xyzzy05 + 4", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("{LD,ST}R (integer register) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (uimm12)\n");
TESTINST2_hide2("ldrsw x21, [x22, #24]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh x21, [x22, #20]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh w21, [x22, #44]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb x21, [x22, #88]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb w21, [x22, #56]", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (simm9, upd) (upd check is MISSING)\n");
TESTINST2_hide2("ldrsw x21, [x22, #-24]!", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh x21, [x22, #-20]!", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh w21, [x22, #-44]!", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb x21, [x22, #-88]!", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb w21, [x22, #-56]!", AREA_MID, x21,x22,0);

TESTINST2_hide2("ldrsw x21, [x22], #-24", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh x21, [x22], #-20", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh w21, [x22], #-44", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb x21, [x22], #-88", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb w21, [x22], #-56", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (simm9, noUpd)\n");
TESTINST2_hide2("ldrsw x21, [x22, #-24]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh x21, [x22, #-20]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsh w21, [x22, #-44]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb x21, [x22, #-88]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldrsb w21, [x22, #-56]", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("LDP,STP (immediate, simm7) (FP&VEC) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("{LD,ST}R (vector register) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (integer register, SX)\n");

TESTINST3_hide2and3("ldrsw x21, [x22,x23]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsw x21, [x22,x23, lsl #2]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsw x21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsw x21, [x22,w23,uxtw #2]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsw x21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsw x21, [x22,w23,sxtw #2]", AREA_MID, -5ULL, x21,x22,x23,0);

TESTINST3_hide2and3("ldrsh x21, [x22,x23]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh x21, [x22,x23, lsl #1]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh x21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh x21, [x22,w23,uxtw #1]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh x21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh x21, [x22,w23,sxtw #1]", AREA_MID, -5ULL, x21,x22,x23,0);

TESTINST3_hide2and3("ldrsh w21, [x22,x23]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh w21, [x22,x23, lsl #1]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh w21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh w21, [x22,w23,uxtw #1]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh w21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsh w21, [x22,w23,sxtw #1]", AREA_MID, -5ULL, x21,x22,x23,0);

TESTINST3_hide2and3("ldrsb x21, [x22,x23]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb x21, [x22,x23, lsl #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb x21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb x21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb x21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb x21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);

TESTINST3_hide2and3("ldrsb w21, [x22,x23]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb w21, [x22,x23, lsl #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb w21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb w21, [x22,w23,uxtw #0]", AREA_MID, 5, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb w21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);
TESTINST3_hide2and3("ldrsb w21, [x22,w23,sxtw #0]", AREA_MID, -5ULL, x21,x22,x23,0);

////////////////////////////////////////////////////////////////
printf("LDR/STR (immediate, SIMD&FP, unsigned offset) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDR/STR (immediate, SIMD&FP, pre/post index) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDUR/STUR (unscaled offset, SIMD&FP) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDR (literal, SIMD&FP) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LD1/ST1 (single structure, no offset) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LD1/ST1 (single structure, post index) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LD{,A}X{R,RH,RB} (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("ST{,L}X{R,RH,RB} (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LDA{R,RH,RB}\n");
TESTINST2_hide2("ldar  x21, [x22]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldar  w21, [x22]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldarh w21, [x22]", AREA_MID, x21,x22,0);
TESTINST2_hide2("ldarb w21, [x22]", AREA_MID, x21,x22,0);

////////////////////////////////////////////////////////////////
printf("STL{R,RH,RB} (entirely MISSING)\n");

} /* end of test_memory() */


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

static void show_block_xor ( UChar* block1, UChar* block2, Int n )
{
   Int i;
   printf("  ");
   for (i = 0; i < n; i++) {
      if (i > 0 && 0 == (i & 15)) printf("\n  ");
      if (0 == (i & 15)) printf("[%3d]  ", i);
      UInt diff = 0xFF & (UInt)(block1[i] - block2[i]);
      if (diff == 0)
         printf(".. ");
      else
         printf("%02x ", diff);
   }
   printf("\n");
}


// In: rand:
//       memory area, xferred vec regs, xferred int regs, 
//     caller spec:
//       addr reg1, addr reg2
//
// Out: memory area, xferred vec regs, xferred int regs, addr reg1, addr reg2
//
// INSN may mention the following regs as containing load/store data:
//     x13 x23 v17 v18 v19 v20
// and
//     x5 as containing the base address
//     x6 as containing an offset, if required
// A memory area is filled with random data, and x13, x23, v17, v18, v19, v20
// are loaded with random data too.  INSN is then executed, with
// x5 set to the middle of the memory area + AREG1OFF, and x6 set to AREG2VAL.
//
// What is printed out: the XOR of the new and old versions of the
// following:
//    the memory area
//    x13 x23 v17 v18 v19 v20
// and the SUB of the new and old values of the following:
//    x5 x6
// If the insn modifies its base register then (obviously) the x5 "new - old"
// value will be nonzero.

#define MEM_TEST(INSN, AREG1OFF, AREG2VAL) { \
  int i; \
  const int N = 256; \
  UChar* area = memalign16(N); \
  UChar area2[N]; \
  for (i = 0; i < N; i++) area[i] = area2[i] = randUChar(); \
  ULong block[12]; \
  /* 0:x13      1:x23      2:v17.d[0] 3:v17.d[1] 4:v18.d[0] 5:v18.d[1] */ \
  /* 6:v19.d[0] 7:v19.d[1] 8:v20.d[0] 9:v20.d[1] 10:x5      11:x6 */ \
  for (i = 0; i < 12; i++) block[i] = randULong(); \
  block[10] = (ULong)(&area[128]) + (Long)(Int)AREG1OFF; \
  block[11] = (Long)AREG2VAL; \
  ULong block2[12]; \
  for (i = 0; i < 12; i++) block2[i] = block[i]; \
  __asm__ __volatile__( \
  "ldr x13, [%0, #0]  ; " \
  "ldr x23, [%0, #8]  ; " \
  "ldr q17, [%0, #16] ; " \
  "ldr q18, [%0, #32] ; " \
  "ldr q19, [%0, #48] ; " \
  "ldr q20, [%0, #64] ; " \
  "ldr x5,  [%0, #80] ; " \
  "ldr x6,  [%0, #88] ; " \
  INSN " ; " \
  "str x13, [%0, #0]  ; " \
  "str x23, [%0, #8]  ; " \
  "str q17, [%0, #16] ; " \
  "str q18, [%0, #32] ; " \
  "str q19, [%0, #48] ; " \
  "str q20, [%0, #64] ; " \
  "str x5,  [%0, #80] ; " \
  "str x6,  [%0, #88] ; " \
  : : "r"(&block[0]) : "x5", "x6", "x13", "x23", \
                       "v17", "v18", "v19", "v20", "memory", "cc" \
  ); \
  printf("%s  with  x5 = middle_of_block+%lld,  x6=%lld\n", \
         INSN, (Long)AREG1OFF, (Long)AREG2VAL); \
  show_block_xor(&area2[0], area, 256); \
  printf("  %016llx  x13      (xor, xfer intreg #1)\n", block[0] ^ block2[0]); \
  printf("  %016llx  x23      (xor, xfer intreg #2)\n", block[1] ^ block2[1]); \
  printf("  %016llx  v17.d[0] (xor, xfer vecreg #1)\n", block[2] ^ block2[2]); \
  printf("  %016llx  v17.d[1] (xor, xfer vecreg #1)\n", block[3] ^ block2[3]); \
  printf("  %016llx  v18.d[0] (xor, xfer vecreg #2)\n", block[4] ^ block2[4]); \
  printf("  %016llx  v18.d[1] (xor, xfer vecreg #2)\n", block[5] ^ block2[5]); \
  printf("  %016llx  v19.d[0] (xor, xfer vecreg #3)\n", block[6] ^ block2[6]); \
  printf("  %016llx  v19.d[1] (xor, xfer vecreg #3)\n", block[7] ^ block2[7]); \
  printf("  %016llx  v20.d[0] (xor, xfer vecreg #3)\n", block[8] ^ block2[8]); \
  printf("  %016llx  v20.d[1] (xor, xfer vecreg #3)\n", block[9] ^ block2[9]); \
  printf("  %16lld  x5       (sub, base reg)\n",     block[10] - block2[10]); \
  printf("  %16lld  x6       (sub, index reg)\n",    block[11] - block2[11]); \
  printf("\n"); \
  free(area); \
  }

static __attribute__((noinline)) void test_memory2 ( void )
{
////////////////////////////////////////////////////////////////
printf("LDR,STR (immediate, uimm12)");
MEM_TEST("ldr  x13, [x5, #24]", -1, 0);
MEM_TEST("ldr  w13, [x5, #20]", 1, 0);
MEM_TEST("ldrh w13, [x5, #44]", 2, 0);
MEM_TEST("ldrb w13, [x5, #56]", 3, 0);
MEM_TEST("str  x13, [x5, #24]", -3, 0);
MEM_TEST("str  w13, [x5, #20]", 5, 0);
MEM_TEST("strh w13, [x5, #44]", 6, 0);
MEM_TEST("strb w13, [x5, #56]", 7, 0);

////////////////////////////////////////////////////////////////
printf("LDUR,STUR (immediate, simm9)\n");
MEM_TEST("ldr x13, [x5], #-24",  0, 0);
MEM_TEST("ldr x13, [x5, #-40]!", 0, 0);
MEM_TEST("ldr x13, [x5, #-48]",  0, 0);
MEM_TEST("str x13, [x5], #-24",  0, 0);
MEM_TEST("str x13, [x5, #-40]!", 0, 0);
MEM_TEST("str x13, [x5, #-48]",  0, 0);

////////////////////////////////////////////////////////////////
printf("LDP,STP (immediate, simm7)\n");
MEM_TEST("ldp x13, x23, [x5], #-24",   0, 0);
MEM_TEST("ldp x13, x23, [x5, #-40]!",  0, 0);
MEM_TEST("ldp x13, x23, [x5, #-40]",   0, 0);
MEM_TEST("stp x13, x23, [x5], #-24",   0, 0);
MEM_TEST("stp x13, x23, [x5, #-40]!",  0, 0);
MEM_TEST("stp x13, x23, [x5, #-40]",   0, 0);

MEM_TEST("ldp w13, w23, [x5], #-24",   0, 0);
MEM_TEST("ldp w13, w23, [x5, #-40]!",  0, 0);
MEM_TEST("ldp w13, w23, [x5, #-40]",   0, 0);
MEM_TEST("stp w13, w23, [x5], #-24",   0, 0);
MEM_TEST("stp w13, w23, [x5, #-40]!",  0, 0);
MEM_TEST("stp w13, w23, [x5, #-40]",   0, 0);

////////////////////////////////////////////////////////////////
printf("LDR (literal, int reg) (DONE ABOVE)\n");

////////////////////////////////////////////////////////////////
printf("{LD,ST}R (integer register) (entirely MISSING)\n");
MEM_TEST("str x13, [x5, x6]",          12, -4);
MEM_TEST("str x13, [x5, x6, lsl #3]",  12, -4);
MEM_TEST("str x13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str x13, [x5, w6, uxtw #3]", 12,  4);
MEM_TEST("str x13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str x13, [x5, w6, sxtw #3]", 12,  -4);
MEM_TEST("ldr x13, [x5, x6]",          12, -4);
MEM_TEST("ldr x13, [x5, x6, lsl #3]",  12, -4);
MEM_TEST("ldr x13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr x13, [x5, w6, uxtw #3]", 12,  4);
MEM_TEST("ldr x13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr x13, [x5, w6, sxtw #3]", 12,  -4);

MEM_TEST("str w13, [x5, x6]",          12, -4);
MEM_TEST("str w13, [x5, x6, lsl #2]",  12, -4);
MEM_TEST("str w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str w13, [x5, w6, uxtw #2]", 12,  4);
MEM_TEST("str w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str w13, [x5, w6, sxtw #2]", 12,  -4);
MEM_TEST("ldr w13, [x5, x6]",          12, -4);
MEM_TEST("ldr w13, [x5, x6, lsl #2]",  12, -4);
MEM_TEST("ldr w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr w13, [x5, w6, uxtw #2]", 12,  4);
MEM_TEST("ldr w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr w13, [x5, w6, sxtw #2]", 12,  -4);

MEM_TEST("strh w13, [x5, x6]",          12, -4);
MEM_TEST("strh w13, [x5, x6, lsl #1]",  12, -4);
MEM_TEST("strh w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("strh w13, [x5, w6, uxtw #1]", 12,  4);
MEM_TEST("strh w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("strh w13, [x5, w6, sxtw #1]", 12,  -4);
MEM_TEST("ldrh w13, [x5, x6]",          12, -4);
MEM_TEST("ldrh w13, [x5, x6, lsl #1]",  12, -4);
MEM_TEST("ldrh w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldrh w13, [x5, w6, uxtw #1]", 12,  4);
MEM_TEST("ldrh w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldrh w13, [x5, w6, sxtw #1]", 12,  -4);

MEM_TEST("strb w13, [x5, x6]",          12, -4);
MEM_TEST("strb w13, [x5, x6, lsl #0]",  12, -4);
MEM_TEST("strb w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("strb w13, [x5, w6, uxtw #0]", 12,  4);
MEM_TEST("strb w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("strb w13, [x5, w6, sxtw #0]", 12,  -4);
MEM_TEST("ldrb w13, [x5, x6]",          12, -4);
MEM_TEST("ldrb w13, [x5, x6, lsl #0]",  12, -4);
MEM_TEST("ldrb w13, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldrb w13, [x5, w6, uxtw #0]", 12,  4);
MEM_TEST("ldrb w13, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldrb w13, [x5, w6, sxtw #0]", 12,  -4);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (uimm12)\n");
MEM_TEST("ldrsw x13, [x5, #24]", -16, 4);
MEM_TEST("ldrsh x13, [x5, #20]", -16, 4);
MEM_TEST("ldrsh w13, [x5, #44]", -16, 4);
MEM_TEST("ldrsb x13, [x5, #72]", -16, 4);
MEM_TEST("ldrsb w13, [x5, #56]", -16, 4);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (simm9, upd) (upd check is MISSING)\n");
MEM_TEST("ldrsw x13, [x5, #-24]!", -16, 4);
MEM_TEST("ldrsh x13, [x5, #-20]!", -16, 4);
MEM_TEST("ldrsh w13, [x5, #-44]!", -16, 4);
MEM_TEST("ldrsb x13, [x5, #-72]!", -16, 4);
MEM_TEST("ldrsb w13, [x5, #-56]!", -16, 4);

MEM_TEST("ldrsw x13, [x5], #-24", -16, 4);
MEM_TEST("ldrsh x13, [x5], #-20", -16, 4);
MEM_TEST("ldrsh w13, [x5], #-44", -16, 4);
MEM_TEST("ldrsb x13, [x5], #-72", -16, 4);
MEM_TEST("ldrsb w13, [x5], #-56", -16, 4);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (simm9, noUpd)\n");
MEM_TEST("ldrsw x13, [x5, #-24]", -16, 4);
MEM_TEST("ldrsh x13, [x5, #-20]", -16, 4);
MEM_TEST("ldrsh w13, [x5, #-44]", -16, 4);
MEM_TEST("ldrsb x13, [x5, #-72]", -16, 4);
MEM_TEST("ldrsb w13, [x5, #-56]", -16, 4);

////////////////////////////////////////////////////////////////
printf("LDP,STP (immediate, simm7) (FP&VEC)\n");

MEM_TEST("stp q17, q18, [x5, 32]",  -16, 4);
MEM_TEST("stp q17, q18, [x5, 32]!", -16, 4);
MEM_TEST("stp q17, q18, [x5], 32",  -16, 4);

MEM_TEST("stp d17, d18, [x5, 32]",  -16, 4);
MEM_TEST("stp d17, d18, [x5, 32]!", -16, 4);
MEM_TEST("stp d17, d18, [x5], 32",  -16, 4);

//MEM_TEST("stp s17, s18, [x5, 32]",  -16, 4);
//MEM_TEST("stp s17, s18, [x5, 32]!", -16, 4);
//MEM_TEST("stp s17, s18, [x5], 32",  -16, 4);

MEM_TEST("ldp q17, q18, [x5, 32]",  -16, 4);
MEM_TEST("ldp q17, q18, [x5, 32]!", -16, 4);
MEM_TEST("ldp q17, q18, [x5], 32",  -16, 4);

MEM_TEST("ldp d17, d18, [x5, 32]",  -16, 4);
MEM_TEST("ldp d17, d18, [x5, 32]!", -16, 4);
MEM_TEST("ldp d17, d18, [x5], 32",  -16, 4);

//MEM_TEST("ldp s17, s18, [x5, 32]",  -16, 4);
//MEM_TEST("ldp s17, s18, [x5, 32]!", -16, 4);
//MEM_TEST("ldp s17, s18, [x5], 32",  -16, 4);

////////////////////////////////////////////////////////////////
printf("{LD,ST}R (vector register)\n");

#if 0
MEM_TEST("str q17, [x5, x6]",          12, -4);
MEM_TEST("str q17, [x5, x6, lsl #4]",  12, -4);
MEM_TEST("str q17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str q17, [x5, w6, uxtw #4]", 12,  4);
MEM_TEST("str q17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str q17, [x5, w6, sxtw #4]", 12,  -4);
MEM_TEST("ldr q17, [x5, x6]",          12, -4);
MEM_TEST("ldr q17, [x5, x6, lsl #4]",  12, -4);
MEM_TEST("ldr q17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr q17, [x5, w6, uxtw #4]", 12,  4);
MEM_TEST("ldr q17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr q17, [x5, w6, sxtw #4]", 12,  -4);
#endif

MEM_TEST("str d17, [x5, x6]",          12, -4);
MEM_TEST("str d17, [x5, x6, lsl #3]",  12, -4);
MEM_TEST("str d17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str d17, [x5, w6, uxtw #3]", 12,  4);
MEM_TEST("str d17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str d17, [x5, w6, sxtw #3]", 12,  -4);
MEM_TEST("ldr d17, [x5, x6]",          12, -4);
MEM_TEST("ldr d17, [x5, x6, lsl #3]",  12, -4);
MEM_TEST("ldr d17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr d17, [x5, w6, uxtw #3]", 12,  4);
MEM_TEST("ldr d17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr d17, [x5, w6, sxtw #3]", 12,  -4);

MEM_TEST("str s17, [x5, x6]",          12, -4);
MEM_TEST("str s17, [x5, x6, lsl #2]",  12, -4);
MEM_TEST("str s17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str s17, [x5, w6, uxtw #2]", 12,  4);
MEM_TEST("str s17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str s17, [x5, w6, sxtw #2]", 12,  -4);
MEM_TEST("ldr s17, [x5, x6]",          12, -4);
MEM_TEST("ldr s17, [x5, x6, lsl #2]",  12, -4);
MEM_TEST("ldr s17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr s17, [x5, w6, uxtw #2]", 12,  4);
MEM_TEST("ldr s17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr s17, [x5, w6, sxtw #2]", 12,  -4);

MEM_TEST("str h17, [x5, x6]",          12, -4);
MEM_TEST("str h17, [x5, x6, lsl #1]",  12, -4);
MEM_TEST("str h17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str h17, [x5, w6, uxtw #1]", 12,  4);
MEM_TEST("str h17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str h17, [x5, w6, sxtw #1]", 12,  -4);
MEM_TEST("ldr h17, [x5, x6]",          12, -4);
MEM_TEST("ldr h17, [x5, x6, lsl #1]",  12, -4);
MEM_TEST("ldr h17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr h17, [x5, w6, uxtw #1]", 12,  4);
MEM_TEST("ldr h17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr h17, [x5, w6, sxtw #1]", 12,  -4);

MEM_TEST("str b17, [x5, x6]",          12, -4);
MEM_TEST("str b17, [x5, x6, lsl #0]",  12, -4);
MEM_TEST("str b17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("str b17, [x5, w6, uxtw #0]", 12,  4);
MEM_TEST("str b17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("str b17, [x5, w6, sxtw #0]", 12,  -4);
MEM_TEST("ldr b17, [x5, x6]",          12, -4);
MEM_TEST("ldr b17, [x5, x6, lsl #0]",  12, -4);
MEM_TEST("ldr b17, [x5, w6, uxtw]",    12,  4);
MEM_TEST("ldr b17, [x5, w6, uxtw #0]", 12,  4);
MEM_TEST("ldr b17, [x5, w6, sxtw]",    12,  4);
MEM_TEST("ldr b17, [x5, w6, sxtw #0]", 12,  -4);

////////////////////////////////////////////////////////////////
printf("LDRS{B,H,W} (integer register, SX)\n");

MEM_TEST("ldrsw x13, [x5,x6]", 12, -4);
MEM_TEST("ldrsw x13, [x5,x6, lsl #2]", 12, -4);
MEM_TEST("ldrsw x13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsw x13, [x5,w6,uxtw #2]", 12, 4);
MEM_TEST("ldrsw x13, [x5,w6,sxtw #0]", 12, 4);
MEM_TEST("ldrsw x13, [x5,w6,sxtw #2]", 12, -4);

MEM_TEST("ldrsh x13, [x5,x6]",  12, -4);
MEM_TEST("ldrsh x13, [x5,x6, lsl #1]",  12, -4);
MEM_TEST("ldrsh x13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsh x13, [x5,w6,uxtw #1]", 12, 4);
MEM_TEST("ldrsh x13, [x5,w6,sxtw #0]", 12, 4);
MEM_TEST("ldrsh x13, [x5,w6,sxtw #1]",  12, -4);

MEM_TEST("ldrsh w13, [x5,x6]",  12, -4);
MEM_TEST("ldrsh w13, [x5,x6, lsl #1]",  12, -4);
MEM_TEST("ldrsh w13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsh w13, [x5,w6,uxtw #1]", 12, 4);
MEM_TEST("ldrsh w13, [x5,w6,sxtw #0]", 12, 4);
MEM_TEST("ldrsh w13, [x5,w6,sxtw #1]",  12, -4);

MEM_TEST("ldrsb x13, [x5,x6]",  12, -4);
MEM_TEST("ldrsb x13, [x5,x6, lsl #0]",  12, -4);
MEM_TEST("ldrsb x13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsb x13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsb x13, [x5,w6,sxtw #0]", 12, 4);
MEM_TEST("ldrsb x13, [x5,w6,sxtw #0]",  12, -4);

MEM_TEST("ldrsb w13, [x5,x6]",  12, -4);
MEM_TEST("ldrsb w13, [x5,x6, lsl #0]",  12, -4);
MEM_TEST("ldrsb w13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsb w13, [x5,w6,uxtw #0]", 12, 4);
MEM_TEST("ldrsb w13, [x5,w6,sxtw #0]", 12, 4);
MEM_TEST("ldrsb w13, [x5,w6,sxtw #0]",  12, -4);


////////////////////////////////////////////////////////////////
printf("LDR/STR (immediate, SIMD&FP, unsigned offset)\n");
MEM_TEST("str q17, [x5, #-32]", 16, 0);
MEM_TEST("str d17, [x5, #-32]", 16, 0);
MEM_TEST("str s17, [x5, #-32]", 16, 0);
//MEM_TEST("str h17, [x5, #-32]", 16, 0);
//MEM_TEST("str b17, [x5, #-32]", 16, 0);
MEM_TEST("ldr q17, [x5, #-32]", 16, 0);
MEM_TEST("ldr d17, [x5, #-32]", 16, 0);
MEM_TEST("ldr s17, [x5, #-32]", 16, 0);
//MEM_TEST("ldr h17, [x5, #-32]", 16, 0);
//MEM_TEST("ldr b17, [x5, #-32]", 16, 0);

////////////////////////////////////////////////////////////////
printf("LDR/STR (immediate, SIMD&FP, pre/post index)\n");
MEM_TEST("str q17, [x5], #-32", 16, 0);
MEM_TEST("str d17, [x5], #-32", 16, 0);
MEM_TEST("str s17, [x5], #-32", 16, 0);
//MEM_TEST("str h17, [x5], #-32", 16, 0);
//MEM_TEST("str b17, [x5], #-32", 16, 0);
MEM_TEST("ldr q17, [x5], #-32", 16, 0);
MEM_TEST("ldr d17, [x5], #-32", 16, 0);
MEM_TEST("ldr s17, [x5], #-32", 16, 0);
//MEM_TEST("ldr h17, [x5], #-32", 16, 0);
//MEM_TEST("ldr b17, [x5], #-32", 16, 0);

MEM_TEST("str q17, [x5, #-32]!", 16, 0);
MEM_TEST("str d17, [x5, #-32]!", 16, 0);
MEM_TEST("str s17, [x5, #-32]!", 16, 0);
//MEM_TEST("str h17, [x5, #-32]!", 16, 0);
//MEM_TEST("str b17, [x5, #-32]!", 16, 0);
MEM_TEST("ldr q17, [x5, #-32]!", 16, 0);
MEM_TEST("ldr d17, [x5, #-32]!", 16, 0);
MEM_TEST("ldr s17, [x5, #-32]!", 16, 0);
//MEM_TEST("ldr h17, [x5, #-32]!", 16, 0);
//MEM_TEST("ldr b17, [x5, #-32]!", 16, 0);


////////////////////////////////////////////////////////////////
printf("LDUR/STUR (unscaled offset, SIMD&FP)\n");
MEM_TEST("str q17, [x5, #-13]", 16, 0);
MEM_TEST("str d17, [x5, #-13]", 16, 0);
MEM_TEST("str s17, [x5, #-13]", 16, 0);
//MEM_TEST("str h17, [x5, #-13]", 16, 0);
//MEM_TEST("str b17, [x5, #-13]", 16, 0);
MEM_TEST("ldr q17, [x5, #-13]", 16, 0);
MEM_TEST("ldr d17, [x5, #-13]", 16, 0);
MEM_TEST("ldr s17, [x5, #-13]", 16, 0);
//MEM_TEST("ldr h17, [x5, #-13]", 16, 0);
//MEM_TEST("ldr b17, [x5, #-13]", 16, 0);

////////////////////////////////////////////////////////////////
printf("LDR (literal, SIMD&FP) (entirely MISSING)\n");

////////////////////////////////////////////////////////////////
printf("LD1/ST1 (single structure, no offset)\n");
MEM_TEST("st1 {v17.2d},  [x5]", 3, 0)
MEM_TEST("st1 {v17.4s},  [x5]", 5, 0)
MEM_TEST("st1 {v17.8h},  [x5]", 7, 0)
MEM_TEST("st1 {v17.16b}, [x5]", 13, 0)
MEM_TEST("st1 {v17.1d},  [x5]", 3, 0)
MEM_TEST("st1 {v17.2s},  [x5]", 5, 0)
MEM_TEST("st1 {v17.4h},  [x5]", 7, 0)
MEM_TEST("st1 {v17.8b},  [x5]", 13, 0)

MEM_TEST("ld1 {v17.2d},  [x5]", 3, 0)
MEM_TEST("ld1 {v17.4s},  [x5]", 5, 0)
MEM_TEST("ld1 {v17.8h},  [x5]", 7, 0)
MEM_TEST("ld1 {v17.16b}, [x5]", 13, 0)
MEM_TEST("ld1 {v17.1d},  [x5]", 3, 0)
MEM_TEST("ld1 {v17.2s},  [x5]", 5, 0)
MEM_TEST("ld1 {v17.4h},  [x5]", 7, 0)
MEM_TEST("ld1 {v17.8b},  [x5]", 13, 0)

////////////////////////////////////////////////////////////////
printf("LD1/ST1 (single structure, post index)\n");
MEM_TEST("st1 {v17.2d},  [x5], #16", 3, 0)
MEM_TEST("st1 {v17.4s},  [x5], #16", 5, 0)
MEM_TEST("st1 {v17.8h},  [x5], #16", 7, 0)
MEM_TEST("st1 {v17.16b}, [x5], #16", 13, 0)
MEM_TEST("st1 {v17.1d},  [x5], #8", 3, 0)
MEM_TEST("st1 {v17.2s},  [x5], #8", 5, 0)
MEM_TEST("st1 {v17.4h},  [x5], #8", 7, 0)
MEM_TEST("st1 {v17.8b},  [x5], #8", 13, 0)

MEM_TEST("ld1 {v17.2d},  [x5], #16", 3, 0)
MEM_TEST("ld1 {v17.4s},  [x5], #16", 5, 0)
MEM_TEST("ld1 {v17.8h},  [x5], #16", 7, 0)
MEM_TEST("ld1 {v17.16b}, [x5], #16", 13, 0)
MEM_TEST("ld1 {v17.1d},  [x5], #8", 3, 0)
MEM_TEST("ld1 {v17.2s},  [x5], #8", 5, 0)
MEM_TEST("ld1 {v17.4h},  [x5], #8", 7, 0)
MEM_TEST("ld1 {v17.8b},  [x5], #8", 13, 0)

////////////////////////////////////////////////////////////////
printf("LD1R (single structure, replicate)\n");
MEM_TEST("ld1r {v17.2d},  [x5]", 3, -5)
MEM_TEST("ld1r {v17.1d},  [x5]", 3, -4)
MEM_TEST("ld1r {v17.4s},  [x5]", 3, -3)
MEM_TEST("ld1r {v17.2s},  [x5]", 3, -2)
MEM_TEST("ld1r {v17.8h},  [x5]", 3, -1)
MEM_TEST("ld1r {v17.4h},  [x5]", 3, 1)
MEM_TEST("ld1r {v17.16b}, [x5]", 3, 2)
MEM_TEST("ld1r {v17.8b},  [x5]", 3, 3)

MEM_TEST("ld1r {v17.2d},  [x5], #8", 3, -5)
MEM_TEST("ld1r {v17.1d},  [x5], #8", 3, -4)
MEM_TEST("ld1r {v17.4s},  [x5], #4", 3, -3)
MEM_TEST("ld1r {v17.2s},  [x5], #4", 3, -2)
MEM_TEST("ld1r {v17.8h},  [x5], #2", 3, -1)
MEM_TEST("ld1r {v17.4h},  [x5], #2", 3, 1)
MEM_TEST("ld1r {v17.16b}, [x5], #1", 3, 2)
MEM_TEST("ld1r {v17.8b},  [x5], #1", 3, 3)

MEM_TEST("ld1r {v17.2d},  [x5], x6", 3, -5)
MEM_TEST("ld1r {v17.1d},  [x5], x6", 3, -4)
MEM_TEST("ld1r {v17.4s},  [x5], x6", 3, -3)
MEM_TEST("ld1r {v17.2s},  [x5], x6", 3, -2)
MEM_TEST("ld1r {v17.8h},  [x5], x6", 3, -1)
MEM_TEST("ld1r {v17.4h},  [x5], x6", 3, 1)
MEM_TEST("ld1r {v17.16b}, [x5], x6", 3, 2)
MEM_TEST("ld1r {v17.8b},  [x5], x6", 3, 3)

////////////////////////////////////////////////////////////////
printf("LD2/ST2 (multiple 2-elem structs to/from 2/regs, post index)"
       " (VERY INCOMPLETE)\n");

MEM_TEST("ld2 {v17.2d, v18.2d}, [x5], #32", 3, 0)
MEM_TEST("st2 {v17.2d, v18.2d}, [x5], #32", 7, 0)

MEM_TEST("ld2 {v17.4s, v18.4s}, [x5], #32", 13, 0)
MEM_TEST("st2 {v17.4s, v18.4s}, [x5], #32", 17, 0)


////////////////////////////////////////////////////////////////
printf("LD1/ST1 (multiple 1-elem structs to/from 2 regs, no offset)"
        " (VERY INCOMPLETE)\n");

MEM_TEST("ld1 {v17.16b, v18.16b}, [x5]", 3, 0)
MEM_TEST("st1 {v17.16b, v18.16b}, [x5]", 7, 0)


////////////////////////////////////////////////////////////////
printf("LD1/ST1 (multiple 1-elem structs to/from 2 regs, post index)"
        " (VERY INCOMPLETE)\n");

MEM_TEST("ld1 {v17.16b, v18.16b}, [x5], #32", 3, 0)
MEM_TEST("st1 {v17.16b, v18.16b}, [x5], #32", 7, 0)


////////////////////////////////////////////////////////////////
printf("LD1/ST1 (multiple 1-elem structs to/from 3 regs, no offset)"
        " (VERY INCOMPLETE)\n");

MEM_TEST("ld1 {v17.16b, v18.16b, v19.16b}, [x5]", 3, 0)
MEM_TEST("st1 {v17.16b, v18.16b, v19.16b}, [x5]", 7, 0)


////////////////////////////////////////////////////////////////
printf("LD3/ST3 (multiple 3-elem structs to/from 3/regs, post index)"
       " (VERY INCOMPLETE)\n");

MEM_TEST("ld3 {v17.2d, v18.2d, v19.2d}, [x5], #48", 13, 0)
MEM_TEST("st3 {v17.2d, v18.2d, v19.2d}, [x5], #48", 17, 0)



} /* end of test_memory2() */

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int main ( void )
{
  if (1) test_memory();
  if (1) test_memory2();
  return 0;
}
