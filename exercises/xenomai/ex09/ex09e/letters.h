

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

extern char letters[128][8];

void loadLetters();
