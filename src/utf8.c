#include <stdio.h>
#include <string.h>

char *testStrings[] = {
    "You should see the Greek word kosme:       \xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5                          |\n",
    "2.1.1  1 byte  (U-00000000):        \x00                                        \n",
    "2.1.2  2 bytes (U-00000080):        \xc2\x80                                       |\n",
    "2.1.3  3 bytes (U-00000800):        \xe0\xa0\x80                                       |\n",
    "2.1.4  4 bytes (U-00010000):        \xf0\x90\x80\x80                                       |\n",
    "2.1.5  5 bytes (U-00200000):        \xf8\x88\x80\x80\x80                                       |\n",
    "2.1.6  6 bytes (U-04000000):        \xfc\x84\x80\x80\x80\x80                                       |\n",
    "2.2.1  1 byte  (U-0000007F):        \x7f                                        \n",
    "2.2.2  2 bytes (U-000007FF):        \xdf\xbf                                       |\n",
    "2.2.3  3 bytes (U-0000FFFF):        \xef\xbf\xbf                                       |\n",
    "2.2.4  4 bytes (U-001FFFFF):        \xf7\xbf\xbf\xbf                                       |\n",
    "2.2.5  5 bytes (U-03FFFFFF):        \xfb\xbf\xbf\xbf\xbf                                       |\n",
    "2.2.6  6 bytes (U-7FFFFFFF):        \xfd\xbf\xbf\xbf\xbf\xbf                                       |\n",
    "2.3.1  U-0000D7FF = ed 9f bf = \xed\x9f\xbf                                            |\n",
    "2.3.2  U-0000E000 = ee 80 80 = \xee\x80\x80                                            |\n",
    "2.3.3  U-0000FFFD = ef bf bd = \xef\xbf\xbd                                            |\n",
    "2.3.4  U-0010FFFF = f4 8f bf bf = \xf4\x8f\xbf\xbf                                         |\n",
    "2.3.5  U-00110000 = f4 90 80 80 = \xf4\x90\x80\x80                                         |\n",
    "3  Malformed sequences                                                        |\n",
    "3.1.1  First continuation byte 0x80: \x80                                      |\n",
    "3.1.2  Last  continuation byte 0xbf: \xbf                                      |\n",
    "3.1.3  2 continuation bytes: \x80\xbf                                             |\n",
    "3.1.4  3 continuation bytes: \x80\xbf\x80                                            |\n",
    "3.1.5  4 continuation bytes: \x80\xbf\x80\xbf                                           |\n",
    "3.1.6  5 continuation bytes: \x80\xbf\x80\xbf\x80                                          |\n",
    "3.1.7  6 continuation bytes: \x80\xbf\x80\xbf\x80\xbf                                         |\n",
    "3.1.8  7 continuation bytes: \x80\xbf\x80\xbf\x80\xbf\x80                                        |\n",
    "   \x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f                                                          |\n",
    "    \x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f                                                          |\n",
    "    \xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf                                                          |\n",
    "    \xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf                                                         |\n",
        "   \xc0 \xc1 \xc2 \xc3 \xc4 \xc5 \xc6 \xc7 \xc8 \xc9 \xca \xcb \xcc \xcd \xce \xcf                                           |\n",
    "    \xd0 \xd1 \xd2 \xd3 \xd4 \xd5 \xd6 \xd7 \xd8 \xd9 \xda \xdb \xdc \xdd \xde \xdf                                          |\n",
    "   \xe0 \xe1 \xe2 \xe3 \xe4 \xe5 \xe6 \xe7 \xe8 \xe9 \xea \xeb \xec \xed \xee \xef                                          |\n",
    "   \xf0 \xf1 \xf2 \xf3 \xf4 \xf5 \xf6 \xf7                                                          |\n",
    "   \xf8 \xf9 \xfa \xfb                                                                  |\n",
    "   \xfc \xfd                                                                      |\n",
    "3.3.1  2-byte sequence with last byte missing (U+0000):     \xc0               |\n",
    "3.3.2  3-byte sequence with last byte missing (U+0000):     \xe0\x80               |\n",
    "3.3.3  4-byte sequence with last byte missing (U+0000):     \xf0\x80\x80               |\n",
    "3.3.4  5-byte sequence with last byte missing (U+0000):     \xf8\x80\x80\x80               |\n",
    "3.3.5  6-byte sequence with last byte missing (U+0000):     \xfc\x80\x80\x80\x80               |\n",
    "3.3.6  2-byte sequence with last byte missing (U-000007FF): \xdf               |\n",
    "3.3.7  3-byte sequence with last byte missing (U-0000FFFF): \xef\xbf               |\n",
    "3.3.8  4-byte sequence with last byte missing (U-001FFFFF): \xf7\xbf\xbf               |\n",
    "3.3.9  5-byte sequence with last byte missing (U-03FFFFFF): \xfb\xbf\xbf\xbf               |\n",
    "3.3.10 6-byte sequence with last byte missing (U-7FFFFFFF): \xfd\xbf\xbf\xbf\xbf               |\n",
    "   \xc0\xe0\x80\xf0\x80\x80\xf8\x80\x80\x80\xfc\x80\x80\x80\x80\xdf\xef\xbf\xf7\xbf\xbf\xfb\xbf\xbf\xbf\xfd\xbf\xbf\xbf\xbf                                                               |\n",
    "3.5.1  fe = \xfe                                                               |\n",
    "3.5.2  ff = \xff                                                               |\n",
    "3.5.3  fe fe ff ff = \xfe\xfe\xff\xff                                                   |\n",
    "4.1.1 U+002F = c0 af             = \xc0\xaf                                        |\n",
    "4.1.2 U+002F = e0 80 af          = \xe0\x80\xaf                                        |\n",
    "4.1.3 U+002F = f0 80 80 af       = \xf0\x80\x80\xaf                                        |\n",
    "4.1.4 U+002F = f8 80 80 80 af    = \xf8\x80\x80\x80\xaf                                        |\n",
    "4.1.5 U+002F = fc 80 80 80 80 af = \xfc\x80\x80\x80\x80\xaf                                        |\n",
    "4.2.1  U-0000007F = c1 bf             = \xc1\xbf                                   |\n",
    "4.2.2  U-000007FF = e0 9f bf          = \xe0\x9f\xbf                                   |\n",
    "4.2.3  U-0000FFFF = f0 8f bf bf       = \xf0\x8f\xbf\xbf                                   |\n",
    "4.2.4  U-001FFFFF = f8 87 bf bf bf    = \xf8\x87\xbf\xbf\xbf                                   |\n",
    "4.2.5  U-03FFFFFF = fc 83 bf bf bf bf = \xfc\x83\xbf\xbf\xbf\xbf                                   |\n",
    "4.3.1  U+0000 = c0 80             = \xc0\x80                                       |\n",
    "4.3.2  U+0000 = e0 80 80          = \xe0\x80\x80                                       |\n",
    "4.3.3  U+0000 = f0 80 80 80       = \xf0\x80\x80\x80                                       |\n",
    "4.3.4  U+0000 = f8 80 80 80 80    = \xf8\x80\x80\x80\x80                                       |\n",
    "4.3.5  U+0000 = fc 80 80 80 80 80 = \xfc\x80\x80\x80\x80\x80                                       |\n",
    "5.1.1  U+D800 = ed a0 80 = \xed\xa0\x80                                                |\n",
    "5.1.2  U+DB7F = ed ad bf = \xed\xad\xbf                                                |\n",
    "5.1.3  U+DB80 = ed ae 80 = \xed\xae\x80                                                |\n",
    "5.1.4  U+DBFF = ed af bf = \xed\xaf\xbf                                                |\n",
    "5.1.5  U+DC00 = ed b0 80 = \xed\xb0\x80                                                |\n",
    "5.1.6  U+DF80 = ed be 80 = \xed\xbe\x80                                                |\n",
    "5.1.7  U+DFFF = ed bf bf = \xed\xbf\xbf                                                |\n",
    "5.2.1  U+D800 U+DC00 = ed a0 80 ed b0 80 = \xed\xa0\x80\xed\xb0\x80                               |\n",
    "5.2.2  U+D800 U+DFFF = ed a0 80 ed bf bf = \xed\xa0\x80\xed\xbf\xbf                               |\n",
    "5.2.3  U+DB7F U+DC00 = ed ad bf ed b0 80 = \xed\xad\xbf\xed\xb0\x80                               |\n",
    "5.2.4  U+DB7F U+DFFF = ed ad bf ed bf bf = \xed\xad\xbf\xed\xbf\xbf                               |\n",
    "5.2.5  U+DB80 U+DC00 = ed ae 80 ed b0 80 = \xed\xae\x80\xed\xb0\x80                               |\n",
    "5.2.6  U+DB80 U+DFFF = ed ae 80 ed bf bf = \xed\xae\x80\xed\xbf\xbf                               |\n",
    "5.2.7  U+DBFF U+DC00 = ed af bf ed b0 80 = \xed\xaf\xbf\xed\xb0\x80                               |\n",
    "5.2.8  U+DBFF U+DFFF = ed af bf ed bf bf = \xed\xaf\xbf\xed\xbf\xbf                               |\n",
    "5.3.1  U+FFFE = ef bf be = \xef\xbf\xbe                                                |\n",
    "5.3.2  U+FFFF = ef bf bf = \xef\xbf\xbf                                                |\n",
    NULL,
};

int utf8_test_valid(const unsigned char * input, const unsigned int length) {
    int current, remaining;
    unsigned char mbhead;
   
    for(current = 0; current < length; current++) {
        remaining = length - (current + 1);
        switch(input[current]) {
            case 0x00 ... 0x7F:
                break;
            case 0x80 ... 0xC1:
                return 0;
            case 0xC2 ... 0xDF:
                if(remaining < 1) return -1;
                if(input[current+1] < 0x80 || input[current+1] > 0xBF) return 0;
                current+=1; 
                break;
            case 0xE0:
                if(remaining < 2) return -(remaining);
                if(input[current+1] < 0xA0 || input[current+1] > 0xBF) return 0;
            case 0xE1 ... 0xEF:
                if(remaining < 2) return -(remaining);
                if(input[current+1] < 0x80 || input[current+1] > 0xBF) return 0;
                if(input[current+2] < 0x80 || input[current+2] > 0xBF) return 0;
                current+=2;
                break;
            case 0xF0:
                if(remaining < 3) return -(remaining);
                if(input[current+1] < 0x90 || input[current+1] > 0xBF) return 0;
            case 0xF1 ... 0xF7:
                if(remaining < 3) return -(remaining);
                if(input[current+1] < 0x80 || input[current+1] > 0xBF) return 0;
                if(input[current+2] < 0x80 || input[current+2] > 0xBF) return 0;
                if(input[current+3] < 0x80 || input[current+3] > 0xBF) return 0;
                current+=3;
                break;
            case 0xF8:
                if(remaining < 4) return -(remaining);
                if(input[current+1] < 0x88 || input[current+1] > 0xBF) return 0;
            case 0xF9 ... 0xFB:
                if(remaining < 4) return -(remaining);
                if(input[current+1] < 0x80 || input[current+1] > 0xBF) return 0;
                if(input[current+2] < 0x80 || input[current+2] > 0xBF) return 0;
                if(input[current+3] < 0x80 || input[current+3] > 0xBF) return 0;
                if(input[current+4] < 0x80 || input[current+4] > 0xBF) return 0;
                current+=4;
                break;
            case 0xFC:
                if(remaining < 5) return -(remaining);
                if(input[current+1] < 0x84 || input[current+1] > 0xBF) return 0;
            case 0xFD:
                if(input[current+1] < 0x80 || input[current+1] > 0xBF) return 0;
                if(input[current+2] < 0x80 || input[current+2] > 0xBF) return 0;
                if(input[current+3] < 0x80 || input[current+3] > 0xBF) return 0;
                if(input[current+4] < 0x80 || input[current+4] > 0xBF) return 0;
                if(input[current+5] < 0x80 || input[current+5] > 0xBF) return 0;
                current+=5;
                break;
                /* multibyte */
            case 0xFE ... 0xFF:
                return 0;
            default:
                printf("Didn't handle %08x!\n", input[current]);
        }
    }
    return 1;
}

main() {
    int i, length;
    for(i = 0; i < sizeof(testStrings) && testStrings[i] != NULL; i++) {
        printf(" %d = %d\n", i, utf8_test_valid(testStrings[i], strlen(testStrings[i])));
    }
}
