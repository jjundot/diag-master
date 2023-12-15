#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef _WIN32
#ifndef __HAVE_LIBEV__
#define __HAVE_LIBUV__
#endif /* #ifndef __HAVE_LIBEV__ */
#else /* #ifdef _WIN32 */
#ifndef __HAVE_LIBUV__
#define __HAVE_LIBEV__
#endif /* #ifndef __HAVE_LIBUV__ */
#endif /* #ifdef _WIN32 */

#endif /* __CONFIG_H__ */
