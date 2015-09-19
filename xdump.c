#include <stdio.h>
main()
	{
	unsigned char buf[8];
	int	i, len;

	while ((len = read(0, buf, sizeof(buf))) > 0)
		{
		for (i = 0; i < len; i++)
			printf("%3d ", buf[i]);
		printf("\n");
		}
	}
