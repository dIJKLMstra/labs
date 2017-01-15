/* 
 * CS:APP Data Lab 
 * 
 * 
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

/* Copyright (C) 1991-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 8.0.0.  Version 8.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2014, plus Amendment 1 (published
   2015-05-15).  */
/* We do not support C11 <threads.h>.  */
//1
/* 
 * thirdBits - return word with every third bit (starting from the LSB) set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int thirdBits(void) {
 int x =(0x49 << 24) + (0x24 << 16) + (0x92 << 8) + 0x49;  //把这个数拆成4份相加
return x;
}
/*
 * isTmin - returns 1 if x is the minimum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmin(int x) {
 return !(x + x + !x);   //Tmin相加为0 再去除0的干扰
}
//2
/* 
 * isNotEqual - return 0 if x == y, and 1 otherwise 
 *   Examples: isNotEqual(5,5) = 0, isNotEqual(4,5) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNotEqual(int x, int y) {
  return !!(x^y);          //如果相同 x^y 为0  再把它变成1或0
}
/* 
 * anyOddBit - return 1 if any odd-numbered bit in word set to 1
 *   Examples anyOddBit(0x5) = 0, anyOddBit(0x7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int anyOddBit(int x){
 return !!(x & (0xAA + (0xAA << 8) + (0xAA << 16) + (0xAA << 24)));  //相当于把x & 0xAAAAAAAA 只要有奇数位是1 该数就非0
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
return ~x + 1;   //由定义得
}
//3
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int a = ((!x) << 31) >> 31;   // 如果x为真 a为0 x为假 a为0xFFFFFFFF
return (~a & y) + (a & z);  //要不输出x 要不输出y
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
 int a = x >> 31;          //先取 x,y,x - y的符号位
 int b = y >> 31;
 int c = (x + ~y + 1)>>31;
 return 1 & ((~a | b | c) & ( a | ~b | ~c));   //如果x为正 y,z为负 或 x 为负 y,z为正  x - y溢出
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
 int a = y >> 31;          //先取 x,y, y -x的符号位
 int b = x >> 31;
 int c = (~x + y + 1)>>31;
 return 1 & (c ^ (!(( a | ~b | ~c) & (~a | b | c))));  //如果y - x 为正 且不溢出 返回1 其他返回0
}
//4
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5) = 0, bitParity(7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int bitParity(int x) {
 x ^= x >> 16;          //把x的各个位相加于最后一位 再返回最后一位
 x ^= x >> 8;
 x ^= x >> 4;
 x ^= x >> 2;
 x ^= x >> 1;
 x &= 1;
return x;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int sig,fm,sm,tm,fom,fim,bit = 0;  //bit为最高位的1的位数
  int a,b,c,d,e;
  int aa,bb,cc,dd,ee;
  int fpart, spart, tpart, fopart, fipart;
  bit += !(!x | !(~x)); 
  sig = x >> 31;          //取符号位   
  x = (~sig & x) + (sig & ~x);      //把x变为正数
  bit += !(x >> 31);           //加上符号位
  fpart = x >> 16;
  fm = !fpart;                   
  aa = (fm << 31) >> 31;
  a = ~aa;
  bit += a &16;                 // 如果右移16位不为0 bit加上16
  x = (a & fpart) + (aa & x);  //如果右移16位为0 x不变 不为0 x右移16
  spart = x >> 8;
  sm = !spart;          //以下同理 
  bb = (sm << 31) >> 31;
  b = ~bb;
  bit += b & 8;
  x = (b & spart) + (bb & x);
  tpart = x >> 4;
  tm = !tpart;
  cc = (tm << 31) >> 31;
  c = ~cc;
  bit += c & 4;
  x = (c & tpart) + (cc & x);
  fopart = x >> 2;
  fom = !fopart;
  dd = (fom << 31) >> 31;
  d = ~dd;
  bit += d & 2; 
  x = (d & fopart) + (dd & x);
  fipart = x >> 1;
  fim = !fipart;
  ee = (fim << 31) >> 31;
  e = ~ee;
  bit += e & 1;
  return bit;
}
//float
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
 unsigned int sig,exp,ret,_uf,bol;
 sig = uf & 0x80000000;  //先求符号位
 _uf = uf + sig;             //把符号位变为0
 exp = (_uf & 0x7F800000) >> 23;     //取浮点数的指数部分
 if(exp == 0xFF || !_uf) return uf;  //如果uf是INF或0 直接返回uf
 bol =(exp < 2);     //bol用来判断指数部分是否为0或1
 ret = (_uf >> 1) + sig;        //对于bol = 1的情况的0.5uf
 if(bol && ((_uf & 3) == 3)) return ret + 1;  //判断舍入
 if(bol) return ret; 
 return _uf - 0x00800000 + sig;  //对于bol = 0的情况的0.5uf
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
 unsigned int sig, i = 31,exp;
 int z = 0,cmp;
 sig = x & 0x80000000;  //先求符号位
 if(!x) return x;       //x如果是0 直接返回
 if(sig) x = -x;        //把符号位变为0
 if(x >> 31) return 0xcf000000;    //变完符号位还是负数就返回...
 while(i){                      //取最高位1的位数exp
   i--;
   if(1 & (x >> i)){
   exp = i; 
   i = 0;
   }
 }
 cmp = 23 - exp;                //判断最高位和23的大小
 if(cmp >> 31){                            //如果exp大于23 判断舍入 再将第exp位右移到23位
  if(exp > 24) z =(x << (33 + cmp));
  x = x>>(~cmp);
  if(((x & 1) && z)||(((x & 3) == 3) && !z)) x++;
  x = x >> 1;
 }
 else  x = x << cmp;                    //如果exp小于23 直接将第exp位左移到23位
 return (x + ((exp + 126) << 23))^sig;        //最后加上指数位和符号位
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  unsigned int sig;
  int cmp,exp,part; 
  sig = uf & 0x80000000; //先取符号位
  uf += sig;            //把符号位变为0
  part = uf >> 23;
  exp = part -127;        //算出最高位1所在位数 
  uf -= (part - 1) << 23;      //把指数部分减掉
  if(exp > 30) return 0x80000000u;
  if(exp < 0) return 0;
  cmp = 23 - exp;          //和23比较
  if(cmp > 0)  uf = uf >> cmp; //如果最高位小于23 左移到第23位
  else uf = uf << (-cmp);   //反之 右移到23位
  if(sig) uf = -uf;   //把符号位还原
  return uf;    
}
