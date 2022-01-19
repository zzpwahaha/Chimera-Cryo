
#ifndef INFLUXDB_EXPORT_H
#define INFLUXDB_EXPORT_H

#ifdef INFLUXDB_STATIC_DEFINE
#  define INFLUXDB_EXPORT
#  define INFLUXDB_NO_EXPORT
#else
#  ifndef INFLUXDB_EXPORT
#    ifdef InfluxDB_EXPORTS
        /* We are building this library */
#      define INFLUXDB_EXPORT 
#    else
        /* We are using this library */
#      define INFLUXDB_EXPORT 
#    endif
#  endif

#  ifndef INFLUXDB_NO_EXPORT
#    define INFLUXDB_NO_EXPORT 
#  endif
#endif

#ifndef INFLUXDB_DEPRECATED
#  define INFLUXDB_DEPRECATED __declspec(deprecated)
#endif

#ifndef INFLUXDB_DEPRECATED_EXPORT
#  define INFLUXDB_DEPRECATED_EXPORT INFLUXDB_EXPORT INFLUXDB_DEPRECATED
#endif

#ifndef INFLUXDB_DEPRECATED_NO_EXPORT
#  define INFLUXDB_DEPRECATED_NO_EXPORT INFLUXDB_NO_EXPORT INFLUXDB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef INFLUXDB_NO_DEPRECATED
#    define INFLUXDB_NO_DEPRECATED
#  endif
#endif

#endif /* INFLUXDB_EXPORT_H */
