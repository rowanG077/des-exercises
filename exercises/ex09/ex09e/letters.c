#include <stdio.h>
#include <stdlib.h>


// load letters
//------------------

/*
  // note: 
  //  - char is by default unsigned on pi 
  //  - unsigned char represents a byte in C
  //  - each letter has 8x8 pixels (including empty boundary offsets)

  // example code: get pixel at column 3 , row 2 
  loadLetters();  // do only once at main function
  unsigned char letter = "X"; 
  char column_byte=letters[letter][3];
  // get row 2  (third row 0,1,2)
  // - shift to right twice
  column_byte = (column_byte >> 1); 
  column_byte = (column_byte >> 1); 
  // - get last bit 
  int bit_value = column_byte & 0x01;

*/
char letters[128][8];

// convert hexidecimal char to int 
// in hexidecimal char : '0' ... 'F' 
// out int : 0-15
char htoi(char i) {
      if (i >= 'A')
          return i - 'A' + 10; //assume A till F
      else return i - '0';
}

void trans(char c[16], char *o) {
    int i, j;
    // columns
    for (i = 0; i < 8; i++) {
        o[i] = 0;
        // rows
        for (j = 0; j < 8; j++) {
            o[i] |= ((c[2*j] | c[2*j+1]) & (1 << 7-i)) ? (1 << j) : 0;
        }
    }
}

void loadLetters() {
    char line[200];
    int lineNum = 0;
    int i = 0;
    FILE *f;
    f = fopen("letters.hex", "r");

    // loop range  [0,127] 
    while (lineNum < 128) {
        // each line represents a character of 8x8 pixels  

        // readline  
        fgets(line, 200, f);

        //translate to 8 ints/chars
        int i;
        char l[8];
        for (i = 5; i < 37; i+=2) {
            //translate hex char to int/char
            l[(i-5)/2] = htoi(line[i]) << 4 | htoi(line[i+1]);
        }
        trans(l, letters[lineNum]);
        lineNum=lineNum+1;
    }
    fclose(f);
}

