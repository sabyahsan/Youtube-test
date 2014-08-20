#ifndef ATTRIBUTES_H_
#define ATTRIBUTES_H_

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
# define UNUSED(x) x
#endif

#endif /* ATTRIBUTES_H_ */
