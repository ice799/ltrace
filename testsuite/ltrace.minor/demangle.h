
class myclass {
  int myint;
 public:
  myclass(int x);
  myclass(void);
  ~myclass();
  static int Fis_i(int bar);
  int Fi_i(int bar);
  /* Overloaded operators */
  void* operator new(size_t);
  void operator delete(void *);
  /* Unary operation.  */
  myclass operator++();// Preincrement
  myclass operator++(int);// Postincrement

  /* Binary operation.  */
  myclass operator+(int);

  /* Assignment */
  myclass& operator=(const myclass& from);
  /* Nested classes */
  class nested {
  public:
    nested();
    ~nested();
    int Fi_i(int bar);
  };
};

class nested {
 public:
  nested();
  ~nested();
  int Fi_i(int bar);
};
