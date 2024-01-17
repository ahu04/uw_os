#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) 
{
  char filename[512] = { 0 };
  getlastcat(filename);
  printf(1, "XV6_TEST_OUTPUT Last catted filename: %s", filename);
  exit();
}
