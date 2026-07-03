#include "ft_traceroute.h"

#include <stdio.h>

void missingHostError()
{
	fprintf(stderr, "Specify \"host\" missing argument.\n");
	exit(2);
}

void badOptionError(char *option, int index)
{
	fprintf(stderr, "Bad option `%s' (argc %d)\n", option, index);
	exit(2);
}

void argumentRequiredError(char *option, int index)
{
	fprintf(stderr, "Option `%s' (argc %d) requires an argument\n", option, index);
	exit(2);
}

void badArgument(char *option, char *argument, int index)
{
	fprintf(stderr, "Cannot handle `%s' option with arg `%s' (argc %d)\n",
		option, argument, index);
	exit(2);
}

void extraArgumentError(char *argument, int index)
{
	fprintf(stderr, "Extra arg `%s' (position 2, argc %c)\n", argument, index);
	exit(2);
}

void fatalError(char *message)
{
	fprintf(stderr, "Fatal error: %s\n", message);
	exit(1);
}

void argumentTooBigError(char *option, long limit)
{
	fprintf(stderr, "`%s' argument cannot be more than %ld\n", option, limit);
	exit(2);
}
