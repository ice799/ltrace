#include <sys/types.h>

static void unsetenv(const char *name)
{
  register char **ep;
  register char **dp;
  const size_t namelen = strlen (name);

  for (dp = ep = __environ; *ep != NULL; ++ep)
    if (memcmp (*ep, name, namelen) || (*ep)[namelen] != '=')
      {
        *dp = *ep;
        ++dp;
      }
  *dp = NULL;

}
