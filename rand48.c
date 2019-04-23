#include"rand48.h"

struct drand48_data __libc_drand48_data;


int
__drand48_iterate (xsubi, buffer)
     unsigned short int xsubi[3];
     struct drand48_data *buffer;
{
  long long X;
  long long result;

  /* Initialize buffer, if not yet done.  */
  if (buffer->__init==0)
    {
      buffer->__a = 0x5deece66dull;
      buffer->__c = 0xb;
      buffer->__init = 1;
    }

  /* Do the real work.  We choose a data type which contains at least
     48 bits.  Because we compute the modulus it does not care how
     many bits really are computed.  */

  X = (long long) xsubi[2] << 32 | (long long) xsubi[1] << 16 | xsubi[0];

  result = X * buffer->__a + buffer->__c;

  xsubi[0] = result & 0xffff;
  xsubi[1] = (result >> 16) & 0xffff;
  xsubi[2] = (result >> 32) & 0xffff;

  return 0;
}

int
__nrand48_r (xsubi, buffer, result)
     unsigned short int xsubi[3];
     struct drand48_data *buffer;
     long int *result;
{
  /* Compute next state.  */
  if (__drand48_iterate (xsubi, buffer) < 0)
    return -1;

  /* Store the result.  */
  if (sizeof (unsigned short int) == 2)
    *result = xsubi[2] << 15 | xsubi[1] >> 1;
  else
    *result = xsubi[2] >> 1;

  return 0;
}

long int
nrand48 (xsubi)
     unsigned short int xsubi[3];
{
  long int result;

  (void) __nrand48_r (xsubi, &__libc_drand48_data, &result);

  return result;
}