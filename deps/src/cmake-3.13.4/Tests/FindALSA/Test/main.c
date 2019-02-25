#include <alsa/global.h>
#include <stdio.h>
#include <string.h>

int main()
{
  printf("Found ALSA version %s, expected version %s\n",
         snd_asoundlib_version(), CMAKE_EXPECTED_ALSA_VERSION);
  return strcmp(snd_asoundlib_version(), CMAKE_EXPECTED_ALSA_VERSION);
}
