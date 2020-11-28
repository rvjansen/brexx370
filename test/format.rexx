say '----------------------------------------'
say 'File format.rexx'
r=0
/* From:
The REXX Language A Practical Approach to Programming
Second Edition, MICHAEL COWLISHAW, 1990
*/

r=r+rtest("format('3',4)","\== '   3'             ",1)
r=r+rtest("format('1.73',4,0)","\== '   2'        ",2)
r=r+rtest("format('1.73',4,3)","\== '   1.730'    ",3)
r=r+rtest("format('-.76',4,1)","\== '  -0.8'      ",4)
r=r+rtest("format('3.03',4)","\== '   3.03'       ",5)
r=r+rtest("format(' - 12.73',,4)","\== '-12.7300' ",6)
r=r+rtest("format(' - 12.73')","\== '-12.73'      ",7)
r=r+rtest("format('0.000')","\== '0'              ",8)
r=r+rtest("format('12345.73',,,2,2)","\== '1.234573E+04' ",9)
r=r+rtest("format('12345.73',,3,,0)","\== '1.235E+4' ",10)
r=r+rtest("format('1.234573',,3,,0)","\== '1.235' ",11)
r=r+rtest("format('123.45',,3,2,0)","\== '1.235E+02' ",12)
r=r+rtest("format('1.2345',,3,2,0)","\== '1.235    ' ",13)
r=r+rtest("format('12345.73',,,3,6)","\== '12345.73' ",14)
r=r+rtest("format('1234567e5',,3,0)","\== '123456700000.000' ",15)

/* the rest of these are not and have been commented out
r=r+rtest("format(12.34)","\== '12.34' ",16)
r=r+rtest("format(12.34,4)","\== '  12.34' ",17)
r=r+rtest("format(12.34,4,4)","\== '  12.3400' ",18)
r=r+rtest("format(12.34,4,1)","\== '  12.3' ",19)
r=r+rtest("format(12.35,4,1)","\== '  12.4' ",20)
r=r+rtest("format(12.34,,4)","\== '12.3400' ",21)
r=r+rtest("format(12.34,4,0)","\== '  12' ",22)
r=r+rtest("format(99.995,3,2)","\== '100.00' ",23)
r=r+rtest("format(0.111,,4)","\== '0.1110' ",24)
r=r+rtest("format(0.0111,,4)","\== '0.0111' ",25)
r=r+rtest("format(0.00111,,4)","\== '0.0011' ",26)
r=r+rtest("format(0.000111,,4)","\== '0.0001' ",27)
r=r+rtest("format(0.0000111,,4)","\== '0.0000' ",28)
r=r+rtest("format(0.00000111,,4)","\== '0.0000' ",29)
r=r+rtest("format(0.555,,4)","\== '0.5550' ",30)
r=r+rtest("format(0.0555,,4)","\== '0.0555' ",31)
r=r+rtest("format(0.00555,,4)","\== '0.0056' ",32)
r=r+rtest("format(0.000555,,4)","\== '0.0006' ",33)
r=r+rtest("format(0.0000555,,4)","\== '0.0001' ",34)
r=r+rtest("format(0.00000555,,4)","\== '0.0000' ",35)
r=r+rtest("format(0.999,,4)","\== '0.9990' ",36)
r=r+rtest("format(0.0999,,4)","\== '0.0999' ",37)
r=r+rtest("format(0.00999,,4)","\== '0.0100' ",38)
r=r+rtest("format(0.000999,,4)","\== '0.0010' ",39)
r=r+rtest("format(0.0000999,,4)","\== '0.0001' ",40)
r=r+rtest("format(0.00000999,,4)","\== '0.0000' ",41)
r=r+rtest("format(0.455,,4)","\== '0.4550' ",42)
r=r+rtest("format(0.0455,,4)","\== '0.0455' ",43)
r=r+rtest("format(0.00455,,4)","\== '0.0046' ",44)
r=r+rtest("format(0.000455,,4)","\== '0.0005' ",45)
r=r+rtest("format(0.0000455,,4)","\== '0.0000' ",46)
r=r+rtest("format(0.00000455,,4)","\== '0.0000' ",47)
r=r+rtest("format(1.00000045,,6)","\== '1.000000' ",48)
r=r+rtest("format(1.000000045,,7)","\== '1.0000001' ",49)
r=r+rtest("format(1.0000000045,,8)","\== '1.00000000' ",50)
r=r+rtest("format(12.34,,,,0)","\== '1.234E+1' ",51)
r=r+rtest("format(12.34,,,3,0)","\== '1.234E+001' ",52)
r=r+rtest("format(12.34,,,3,)","\== '12.34' ",53)
r=r+rtest("format(1.234,,,3,0)","\== '1.234     ' ",54)
r=r+rtest("format(12.34,3,,,0)","\== '  1.234E+1' ",55)
r=r+rtest("format(12.34,,2,,0)","\== '1.23E+1' ",56)
r=r+rtest("format(12.34,,3,,0)","\== '1.234E+1' ",57)
r=r+rtest("format(12.34,,4,,0)","\== '1.2340E+1' ",58)
r=r+rtest("format(12.345,,3,,0)","\== '1.235E+1' ",59)
r=r+rtest("format(99.999,,,,)","\== '99.999' ",60)
r=r+rtest("format(99.999,,2,,)","\== '100.00' ",61)
/* These from Kurt Maerker */
r=r+rtest("format(99.999,,2,,2)","\== '1.00E+2' ",62)
r=r+rtest("format(.999999,,4,2,2)","\== '1.0000' ",63)
r=r+rtest("format(.999999,,5,2,2)","\== '1.00000' ",64)
r=r+rtest("format(.9999999,,5,2,2)","\== '1.00000' ",65)
r=r+rtest("format(.999999,,6,2,2)","\== '0.999999' ",66)
r=r+rtest("format(90.999,,0)","\== '91' ",67)
r=r+rtest("format(0099.999,5,3,,)","\== '   99.999' ",68)
r=r+rtest("format(0.0000000000000000001,4)","\== '   1E-19' ",69)
r=r+rtest("format(0.0000000000000000001,4,4)","\== '   1.0000E-19' ",70)
r=r+rtest("format(0.0000001,4,,,3)","\== '   1E-7' ",71)
r=r+rtest("format(0.0000001,4,4,,3)","\== '   1.0000E-7' ",72)
r=r+rtest("format(0.000001,4,4,,3)","\== '   0.0000' ",73)
r=r+rtest("format(0.0000001,4,5,,2)","\== '   1.00000E-7' ",74)
r=r+rtest("format(0.0000001,4,4,4,3)","\== '   1.0000E-0007' ",75)
r=r+rtest("format(1000,4,4,,3)","\== '   1.0000E+3' ",76)
r=r+rtest("format(0.0000000000000000000001)","\== '1E-22' ",77)
r=r+rtest("format(0.0000001,,,0,3)","\== '0.0000001' ",78)
r=r+rtest("format('.00001',,,2,9)","\== '0.00001' ",79)
r=r+rtest("format('.000001',,,2,9)","\== '0.000001' ",80)
r=r+rtest("format('.0000001',,,2,9)","\== '1E-07' ",81)
r=r+rtest("format('.00000001',,,2,9)","\== '1E-08' ",82)
r=r+rtest("format(99.999,,2,,2)","\== '1.00E+2' ",83)
r=r+rtest("format(.999999,,4,2,2)","\== '1.0000' ",84)
r=r+rtest("format(.9999999,,5,2,2)","\== '1.00000' ",85)
r=r+rtest("format('.0000001',,,2,9)","\== '1E-07' ",86)
r=r+rtest("format('.00000001',,,2,9)","\== '1E-08' ",87)
r=r+rtest("format(9.9999999,1,10,1,1)","\== '9.9999999000' ",88)
r=r+rtest("format(9.9999999,1,10,1,2)","\== '9.9999999000' ",89)
r=r+rtest("format(9.9999999,1,10,2,1)","\== '9.9999999000' ",90)
r=r+rtest("format(9.9999999,1,10,2,2)","\== '9.9999999000' ",91)
r=r+rtest("format(9.9999999,1,10,2,3)","\== '9.9999999000' ",92)
r=r+rtest("format(9.9999999,1,10,4,3)","\== '9.9999999000' ",93)
r=r+rtest("format(9.9999999,1,8,1,1)","\== '9.99999990' ",94)
r=r+rtest("format(9.9999999,1,8,1,2)","\== '9.99999990' ",95)
r=r+rtest("format(9.99999999,1,10,1,1)","\== '9.9999999900' ",96)
r=r+rtest("format(9.99999999,1,10,1,2)","\== '9.9999999900' ",97)
r=r+rtest("format(9.99999999,1,10,1,3)","\== '9.9999999900' ",98)
r=r+rtest("format(9.99999999,1,10,2,1)","\== '9.9999999900' ",99)
r=r+rtest("format(9.99999999,1,10,2,2)","\== '9.9999999900' ",100)
r=r+rtest("format(9.99999999,1,10,2,3)","\== '9.9999999900' ",101)
r=r+rtest("format(9.99999999,1,10,3,1)","\== '9.9999999900' ",102)
r=r+rtest("format(9.99999999,1,10,3,2)","\== '9.9999999900' ",103)
r=r+rtest("format(9.99999999,1,10,3,3)","\== '9.9999999900' ",104)
r=r+rtest("format(9.99999999,1,10,4,3)","\== '9.9999999900' ",105)
r=r+rtest("format(9.99999999,1,10,5,3)","\== '9.9999999900' ",106)
r=r+rtest("format(9.99999999,1,8,1,1)","\== '9.99999999' ",107)
r=r+rtest("format(9.99999999,1,8,1,2)","\== '9.99999999' ",108)
r=r+rtest("format(9.99999999,1,8,2,1)","\== '9.99999999' ",109)
r=r+rtest("format(0.0000000000000000000001,,,0,)",, 
"\== '0.0000000000000000000001'",110)
*/
say 'Done format.rexx'
exit r
