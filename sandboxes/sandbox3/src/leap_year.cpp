#include "leap_year.h"

// bool IsLeapYear(int year) {
//     /*
//      * Год високосный, если он делится на 4 без остатка.
//      * Если год делится на 100, то он не високосный.
//      * Если год делится на 400, то он високосный.
//      */
//     return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
// }


bool IsLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) /* || (year % 400 == 0) */;
}  