#include <gm_metric.h>
#include <libmetrics.h>
#include <apr_strings.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>

mmodule ibcounters_module;

#ifndef IB_STATS_BASE_DIR
#define IB_STATS_BASE_DIR "/sys/class/infiniband"
#endif

#ifndef IB_STATS_CHECK_FREQUENCY
#define IB_STATS_CHECK_FREQUENCY (0.5) /* in seconds */
#endif

/*!
    @enumerate InfiniBand counter indexes
    
    Enumerates the counters that are tracked by this module, with
    the final value (kIBMaxCounterIdx) representing the number
    of counters present.
*/
enum {
    kIBTxPktCounterIdx     = 0,
    kIBTxWordsCounterIdx,
    kIBTxErrCounterIdx,
    kIBTxMulticastCounterIdx,

    kIBRxPktCounterIdx,
    kIBRxWordsCounterIdx,
    kIBRxErrCounterIdx,
    kIBRxMulticastCounterIdx,

    kIBBufferOverrunErrCounterIdx,

    kIBIBSymbolErrCounterIdx,

    kIBTxDroppedCounterIdx,

    kIBMaxCounterIdx
};

/*!
    @enumerate Recognized InfiniBand driver types
    
    Enumerates the InfiniBand drivers that this module is able to handle.
    The driver types are inferred from the directory name present under
    /sys/class/infiniband, e.g. mlx4_0.
    
    The final value (kIBDriverMax) represents the number of driver types
    present.
*/
enum {
    kIBDriverMlx4 = 0,
    kIBDriverMlx5,
    kIBDriverMax
};

/*!
    @constant IBDriverNamePatterns
    
    For each InfiniBand driver type a fnmatch() pattern is provided to
    identify a /sys/class/infiniband subdirectory as being of the
    type.
    
    Ordered to match the InfiniBand driver type enumeration.
*/
static const char* IBDriverNamePatterns[kIBDriverMax] =
{
    "mlx4_*",
    "mlx5_*"
};

/*!
    @enumerate InfiniBand counter types
    
    Enumerates the counter types -- the behavior of the counter value.  A
    counter can simply report the current value (useful for error counters)
    or a time-based rate of change for counters that should not be treated
    as cumulative (bytes sent per second).
*/
enum {
    kIBCounterTypeCount = 0,
    kIBCounterTypeRate
};

/*!
    @typedef IBMetricDescriptor
    
    A metric descriptor wraps a subpath (relative to the base directory
    /sys/class/infiniband/<device>/ports/<port#>) that should be read
    to obtain the counter value; a Ganglia metric definition struct that
    acts as a template for per-device-port metrics that are reported; and
    the counter type.
    
    
*/
typedef struct {
    const char                      *subpath;
    Ganglia_25metric                metricTemplate;
    int                             counterType;
} IBMetricDescriptor;

/*!
    @constant IBMetricDescriptors_mlx4
    
    Metric descriptors for the mlx4 driver.
*/
static IBMetricDescriptor IBMetricDescriptors_mlx4[kIBMaxCounterIdx] = {
        {
            "counters_ext/port_xmit_packets_64",
            {0, "%s_p%ld_TxPkt",            0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Packets transmitted (in packets per second)"},
            kIBCounterTypeRate
        },
        {
            "counters_ext/port_xmit_data_64",
            {0, "%s_p%ld_TxWords",          0, GANGLIA_VALUE_DOUBLE, "word/s",  "both", "%.3f", UDP_HEADER_SIZE+16, "Words transmitted (in words per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_xmit_constraint_errors",
            {0, "%s_p%ld_TxErrs",           0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Transmit error count"},
            kIBCounterTypeCount
        },
        {
            "counters_ext/port_multicast_xmit_packets",
            {0, "%s_p%ld_TxMulticast",      0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Multicast packets transmitted (in packet per second)"},
            kIBCounterTypeRate
        },
    
        {
            "counters_ext/port_rcv_packets_64",
            {0, "%s_p%ld_RxPkt",            0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Packets received (in packets per second)"},
            kIBCounterTypeRate
        },
        {
            "counters_ext/port_rcv_data_64",
            {0, "%s_p%ld_RxWords",          0, GANGLIA_VALUE_DOUBLE, "word/s",  "both", "%.3f", UDP_HEADER_SIZE+16, "Words received (in words per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_rcv_errors",
            {0, "%s_p%ld_RxErrs",           0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Receive error count"},
            kIBCounterTypeCount
        },
        {
            "counters_ext/port_multicast_rcv_packets",
            {0, "%s_p%ld_RxMulticast",      0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Multicast packets received (in packet per second)"},
            kIBCounterTypeRate
        },
    
        {
            "counters/excessive_buffer_overrun_errors",
            {0, "%s_p%ld_BufferOverrunErr", 0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Buffer overrun error count"},
            kIBCounterTypeCount
        },
    
        {
            "counters/symbol_error",
            {0, "%s_p%ld_IBSymbolErr",      0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Symbol error count"},
            kIBCounterTypeCount
        },
    
        {
            "counters/port_xmit_discards",
            {0, "%s_p%ld_TxDropped",        0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Dropped transmit count"},
            kIBCounterTypeCount
        }
    };

/*!
    @constant IBMetricDescriptors_mlx5
    
    Metric descriptors for the mlx5 driver.
*/
static IBMetricDescriptor IBMetricDescriptors_mlx5[kIBMaxCounterIdx] = {
        {
            "counters/port_xmit_packets",
            {0, "%s_p%ld_TxPkt",            0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Packets transmitted (in packets per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_xmit_data",
            {0, "%s_p%ld_TxWords",          0, GANGLIA_VALUE_DOUBLE, "word/s",  "both", "%.3f", UDP_HEADER_SIZE+16, "Words transmitted (in words per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_xmit_constraint_errors",
            {0, "%s_p%ld_TxErrs",           0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Transmit error count"},
            kIBCounterTypeCount
        },
        {
            "counters/multicast_xmit_packets",
            {0, "%s_p%ld_TxMulticast",      0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Multicast packets transmitted (in packet per second)"},
            kIBCounterTypeRate
        },
    
        {
            "counters/port_rcv_packets",
            {0, "%s_p%ld_RxPkt",            0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Packets received (in packets per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_rcv_data",
            {0, "%s_p%ld_RxWords",          0, GANGLIA_VALUE_DOUBLE, "word/s",  "both", "%.3f", UDP_HEADER_SIZE+16, "Words received (in words per second)"},
            kIBCounterTypeRate
        },
        {
            "counters/port_rcv_errors",
            {0, "%s_p%ld_RxErrs",           0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Receive error count"},
            kIBCounterTypeCount
        },
        {
            "counters/multicast_rcv_packets",
            {0, "%s_p%ld_RxMulticast",      0, GANGLIA_VALUE_DOUBLE, "pkt/s",   "both", "%.3f", UDP_HEADER_SIZE+16, "Multicast packets received (in packet per second)"},
            kIBCounterTypeRate
        },
    
        {
            "counters/excessive_buffer_overrun_errors",
            {0, "%s_p%ld_BufferOverrunErr", 0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Buffer overrun error count"},
            kIBCounterTypeCount
        },
    
        {
            "counters/symbol_error",
            {0, "%s_p%ld_IBSymbolErr",      0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Symbol error count"},
            kIBCounterTypeCount
        },
    
        {
            "counters/port_xmit_discards",
            {0, "%s_p%ld_TxDropped",        0, GANGLIA_VALUE_DOUBLE, "",        "both", "%.0f", UDP_HEADER_SIZE+16, "Dropped transmit count"},
            kIBCounterTypeCount
        }
    };

/*!
    @constant IBMetricDescriptors
    
    Array of pointers to each driver's metric descriptor definitions.
    
    Ordered to match the InfiniBand driver type enumeration.
*/
static IBMetricDescriptor* IBMetricDescriptors[kIBDriverMax] = {
        IBMetricDescriptors_mlx4,
        IBMetricDescriptors_mlx5
    };

/*!
    @enumerate Tri-state of a read counter
    
    Each counter can be in one of three states:
    
    - unknown:  initial state implying the counter has not been read
    - inited:   for rate-based counters, indicates a value has been
                read but the rate cannot be reported yet
    - valued:   the counter has a value that can be returned for the
                metric
    
    Each counter starts in the unknown state.  On a successful read,
    a rate-based counter transitions to inited while a simple-valued
    counter transitions immediately to valued.  A subsequent successful
    read transition a rate-based counter to valued.  A failed read
    transitions the counter back to unknown.
*/
enum {
    kIBFieldStateUnknown = 0,
    kIBFieldStateInited,
    kIBFieldStateValued
};

/*!
    @typedef IBCounterField
    
    Stateful representation of an InfiniBand counter.  The fieldState
    is used for stateful transition of the counter toward providing a
    value to gmond.  The currentValue contains the counter value or
    rate-of-change associated with the counter value, dependent on the
    type of counter.
    
    The lastReadValue holds the previously-read counter value (regardless
    of the counter type) and lastReadTime the system timestamp
    associated with that value.
*/
typedef struct {
    int             fieldState;
    double          currentValue;
    double          lastReadValue;
    struct timeval  lastReadTime;
} IBCounterField;

/*!
    @typedef IBDevicePort
    
    Linked-list element that wraps a recognized InfiniBand device-port
    with its driver's metric descriptor templates and the set of counter
    fields' state records.
*/
typedef struct __IBDevicePort {
    /* Link to next record: */
    struct __IBDevicePort   *link;
    
    /* Device id: */
    const char              *devName;
    long                    devPort;
    
    /* Metric descriptor template: */
    IBMetricDescriptor      *metricDescriptors;
    
    /* Counter fields: */
    IBCounterField          fields[kIBMaxCounterIdx];
} IBDevicePort;

/*!
    @function IBDevicePortAlloc
    
    Given the /sys/class/infiniband/<devName>/ports/<devPort> for an
    InfiniBand device-port, locate the appropriate per-driver metric
    descriptors.  If successful, allocates a new IBDevicePort element
    and fills-in the device identification and metric descriptor
    fields.
    
    The entire data structure is zeroed, which leaves the counter fields
    in an appropriately-initialized state.
    
    In case of any error, NULL is retured.  Otherwise, a pointer to
    the allocated and initialized IBDevicePort element is returned.
 */
static IBDevicePort*
IBDevicePortAlloc(
    const char  *devName,
    long        devPort
)
{
    IBDevicePort    *newDevicePort =  NULL;
    int             driverIdx = kIBDriverMlx4;
    
    /* Figure out which driver it is: */
    while ( driverIdx < kIBDriverMax ) {
        if ( fnmatch(IBDriverNamePatterns[driverIdx], devName, 0) == 0 ) break;
        driverIdx++;
    }
    if ( driverIdx < kIBDriverMax ) {
        size_t      newDevicePortSize = sizeof(IBDevicePort) + strlen(devName) + 1;
        
        newDevicePort = (IBDevicePort*)malloc(newDevicePortSize);
        if ( newDevicePort ) {
            memset(newDevicePort, 0, newDevicePortSize);
        
            newDevicePort->devName = ((void*)newDevicePort) + sizeof(IBDevicePort);
            strcpy((char*)newDevicePort->devName, devName);
        
            newDevicePort->devPort = devPort;
            newDevicePort->metricDescriptors = IBMetricDescriptors[driverIdx];
        }
    } else {
        debug_msg("[ibcounters] unknown driver '%s'", devName);
    }
    return newDevicePort;
}

/*!
    @function __IBDevicePortReadCounter
    
    Construct the path to the device-port's counter (at counterPath) and read a
    double-precision floating-point value from that file if it exists.
    
    Returns non-zero if the counter was read, zero otherwise.
 */
static int
__IBDevicePortReadCounter(
    IBDevicePort    *devToRead,
    const char      *counterPath,
    double          *counterValue
)
{
    FILE            *fptr;
    int             rc = 0;
    char            path[PATH_MAX];

    if ( snprintf(path, sizeof(path), IB_STATS_BASE_DIR "/%s/ports/%ld/%s", devToRead->devName, devToRead->devPort, counterPath) < sizeof(path) ) {
        if ( (fptr = fopen(path, "r")) ) {
            rc = ( fscanf(fptr, "%lg", counterValue) == 1 );
            debug_msg("[ibcounters] read counter '%s' => %lg", path, *counterValue);
            fclose(fptr);
        }
    }
    return rc;
}

/*!
    @function IBDevicePortReadCounters
    
    Attempt to read all counters associated with a device-port.  Each field's
    state is progressed independently, so failure to read one counter does not
    prevent the reporting of others.
    
    On exit, any field associated with devToRead in state kIBFieldStateValued
    can be reported to gmond.
    
    The compile-time constant IB_STATS_CHECK_FREQUENCY determines how often a
    counter will be rechecked, regardless of the granularity configured in
    gmond.
 */
static void
IBDevicePortReadCounters(
    IBDevicePort        *devToRead
)
{
    int                 rc = 0;
    struct timeval      currentTime;
    int                 counterIdx = 0;
    
    while ( counterIdx < kIBMaxCounterIdx ) {
        gettimeofday(&currentTime, NULL);
        
        /* What do we need to do? */
        switch ( devToRead->fields[counterIdx].fieldState ) {
        
            case kIBFieldStateUnknown: {
                double      value;
                
                /* Force a read: */
                if ( __IBDevicePortReadCounter(devToRead, devToRead->metricDescriptors[counterIdx].subpath, &value) ) {
                    devToRead->fields[counterIdx].lastReadValue = value;
                    devToRead->fields[counterIdx].currentValue = value;
                    devToRead->fields[counterIdx].lastReadTime = currentTime;
                    /* If this is a simple counter, we can transition right to "value" state: */
                    if ( devToRead->metricDescriptors[counterIdx].counterType == kIBCounterTypeCount ) {
                        devToRead->fields[counterIdx].fieldState = kIBFieldStateValued;
                    } else {
                        devToRead->fields[counterIdx].fieldState = kIBFieldStateInited;
                    }
                }
                break;
            }
            
            case kIBFieldStateInited:
            case kIBFieldStateValued: {
                double      dt = ((double)currentTime.tv_sec * 1.0e6 +
                                      (double)currentTime.tv_usec -
                                      (double)devToRead->fields[counterIdx].lastReadTime.tv_sec * 1.0e6 -
                                      (double)devToRead->fields[counterIdx].lastReadTime.tv_usec) * 1.0e-6;
            
                if ( dt >= IB_STATS_CHECK_FREQUENCY ) {
                    double  value;
                
                    /* Force a read: */
                    if ( __IBDevicePortReadCounter(devToRead, devToRead->metricDescriptors[counterIdx].subpath, &value) ) {
                        switch ( devToRead->metricDescriptors[counterIdx].counterType ) {
                            case kIBCounterTypeCount:
                                devToRead->fields[counterIdx].currentValue = value;
                                break;
                            case kIBCounterTypeRate:
                                devToRead->fields[counterIdx].currentValue = ( value - devToRead->fields[counterIdx].lastReadValue ) / dt;
                                break;
                        }
                        devToRead->fields[counterIdx].lastReadValue = value;
                        devToRead->fields[counterIdx].lastReadTime = currentTime;
                        devToRead->fields[counterIdx].fieldState = kIBFieldStateValued;
                    } else {
                        /* Failed to read the counter, so fall back to unknown state: */
                        devToRead->fields[counterIdx].fieldState = kIBFieldStateUnknown;
                    }
                }
            }
        }
        counterIdx++;
    }
}

/*!
    @constant IBDevicePortsHead
    
    Head of the device-port linked list created by this module.
*/
static IBDevicePort *IBDevicePortsHead = NULL;

/*!
    @constant IBDevicePortsCount
    
    The number of device-port elements present in the IBDevicePortsHead linked list.
*/
static int          IBDevicePortsCount = 0;

/*!
    @function IBDevicePortsInit
    
    Determine how many IB ports are present and allocate state storage for each.
    
    Devices are found in /sys/class/infiniband, e.g. in subdirectories like mlx5_0/.
    Ports are present in a ports/ subdirectory of the device directory, e.g.
    /sys/class/infiniband/mlx5_0/ports/1.
    
    Returns non-zero on failure, zero if successful.
 */
static int
IBDevicePortsInit(void)
{
    DIR                             *dptr = opendir(IB_STATS_BASE_DIR);
    
    debug_msg("[ibcounters] entered IBDevicePortsInit()");
    
    if ( dptr ) {
        struct dirent               *edir;
        char                        fullpath[PATH_MAX];
        
        while ( (edir = readdir(dptr)) ) {
            if ( (strcmp(edir->d_name, ".") == 0) || (strcmp(edir->d_name, "..") == 0) ) continue;
            if ( snprintf(fullpath, sizeof(fullpath), IB_STATS_BASE_DIR "/%s/ports", edir->d_name) < sizeof(fullpath) ) {
                DIR                 *pdptr = opendir(fullpath);
                
                if ( pdptr ) {
                    struct dirent   *epdir;
                    
                    debug_msg("[ibcounters]  -> walking '%s'", fullpath);
                    while ( (epdir = readdir(pdptr)) ) {
                        IBDevicePort  *newDevicePort = NULL;
                        long        devPort;
                        char        *endptr = NULL;
                        
                        if ( (strcmp(epdir->d_name, ".") == 0) || (strcmp(epdir->d_name, "..") == 0) ) continue;
                        devPort = strtol(epdir->d_name, &endptr, 10);
                        if ( (devPort > 0) && (endptr > epdir->d_name) ) {                    
                            debug_msg("[ibcounters]      found port %ld", devPort);
                            
                            /* Found a port: */
                            newDevicePort = IBDevicePortAlloc(edir->d_name, devPort);
                            if ( ! newDevicePort ) return 1;
                            IBDevicePortsCount++;
                            newDevicePort->link = IBDevicePortsHead;
                            IBDevicePortsHead = newDevicePort;
                        }
                    }
                    closedir(pdptr);
                }
            }
        }
        closedir(dptr);
    }
    
    debug_msg("[ibcounters] exiting IBDevicePortsInit()");
    return 0;
}

/*!
    @function IBDevicePortsDestroy
    
    Deallocate the entire list of device-ports.
 */
static void
IBDevicePortsDestroy(void)
{
    IBDevicePort      *p = IBDevicePortsHead;
    
    debug_msg("[ibcounters] entering IBDevicePortsDestroy()");
    while ( p ) {
        IBDevicePort  *next = p->link;
        
        free((void*)p);
        p = next;
    }
    IBDevicePortsHead = NULL;
    IBDevicePortsCount = 0;
    debug_msg("[ibcounters] exiting IBDevicePortsDestroy()");
}

/*!
    @constant gangliaMetricDescriptorArray
    
    An APR array holding all of the concrete Ganglia metric descriptors
    for this module.  The descriptors must be constructed dynamically
    after the zero or more device-ports are discovered by
    IBDevicePortsInit().
    
    See IBDevicePortsRegisterGMetrics().
*/
static apr_array_header_t   *gangliaMetricDescriptorArray = NULL;

/*!
    @function IBDevicePortsRegisterGMetrics
    
    Register concrete Ganglia metric descriptors for each of the
    device-ports.
 */
static void
IBDevicePortsRegisterGMetrics(
    apr_pool_t      *parentPool
)
{
    IBDevicePort        *p = IBDevicePortsHead;
    const char          metricName[PATH_MAX];
    apr_pool_t          *ourPool;
    Ganglia_25metric    *newMetric;
    int                 counterIdx;
    
    debug_msg("[ibcounters] entered IBDevicePortsRegisterGMetrics()");
    
    /* Allocate a pool that will be used by this module */
    apr_pool_create(&ourPool, parentPool);
    
    /* Setup the table of metric descriptors: */
    gangliaMetricDescriptorArray = apr_array_make(ourPool, (IBDevicePortsCount * kIBMaxCounterIdx) + 1, sizeof(Ganglia_25metric));
    debug_msg("[ibcounters]  -> descriptor table created = %p", gangliaMetricDescriptorArray);
    
    while ( p ) {
        counterIdx = 0;
        while ( counterIdx < kIBMaxCounterIdx ) {
            newMetric = (Ganglia_25metric*)apr_array_push(gangliaMetricDescriptorArray);
            *newMetric = p->metricDescriptors[counterIdx].metricTemplate;
            newMetric->name = apr_psprintf (ourPool, p->metricDescriptors[counterIdx].metricTemplate.name, p->devName, p->devPort);
            debug_msg("[ibcounters]  -> metric allocated '%s' = %p", newMetric->name, newMetric);
            counterIdx++;
        }
        p = p->link;
    }
    
    /* Push a null record onto the list: */
    newMetric = (Ganglia_25metric*)apr_array_push(gangliaMetricDescriptorArray);
    memset(newMetric, 0, sizeof(*newMetric));
    debug_msg("[ibcounters]  -> NULL metric allocated = %p", newMetric);
    
    /* Fill-in the module struct: */
    ibcounters_module.metrics_info = (Ganglia_25metric*)gangliaMetricDescriptorArray->elts;
    
    /* Configure metadata on each metric: */
    counterIdx = 0;
    while ( ibcounters_module.metrics_info[counterIdx].name != NULL ) {
        MMETRIC_INIT_METADATA(&(ibcounters_module.metrics_info[counterIdx]), parentPool);
        MMETRIC_ADD_METADATA(&(ibcounters_module.metrics_info[counterIdx]), MGROUP, "infiniband");
        debug_msg("[ibcounters]  -> metric metadata inited '%s'", ibcounters_module.metrics_info[counterIdx].name);
        counterIdx++;
    }
    debug_msg("[ibcounters] exiting IBDevicePortsRegisterGMetrics()");
}
        
/*!
    @function IBDevicePortsReadCounters
    
    Attempt to read all device-ports' counters to update rates/counters.
 */
static void
IBDevicePortsReadCounters(void)
{
    IBDevicePort    *p = IBDevicePortsHead;
    
    debug_msg("[ibcounters] entered IBDevicePortsReadCounters()");
    while ( p ) {
        IBDevicePortReadCounters(p);
        p = p->link;
    }
    debug_msg("[ibcounters] exiting IBDevicePortsReadCounters()");
}

/*!
    @function ibcounters_metric_init
    
    Ganglia metric callback function that initializes this module.
    
    Scans for all recognizable InfiniBand device-ports under
    /sys/class/infiniband and creates state structures.  If
    successful, registers all derived metrics with gmond to begin
    monitoring.
    
    Returns 0 on success, non-zero otherwise.
*/
static int
ibcounters_metric_init (
  apr_pool_t    *p
)
{
    int i;
    
    /* Initialize the ganglia library: */
    libmetrics_init();

    debug_msg("[ibcounters] entered ibcounters_metric_init()");

    /* See if we have any Infiniband devices present: */
    if ( IBDevicePortsInit() != 0 ) return 1;

    /* Register all metrics: */
    IBDevicePortsRegisterGMetrics(p);
    
    /* Initial read of the counters: */
    IBDevicePortsReadCounters();

    debug_msg("[ibcounters] exiting ibcounters_metric_init()");
    return 0;
}

/*!
    @function ibcounters_metric_cleanup
    
    Ganglia metric callback function that shuts down this module.
    
    Deallocates all of the in-memory state structures for the
    discovered device-ports.
*/
static void
ibcounters_metric_cleanup(void)
{
    debug_msg("[ibcounters] entered ibcounters_metric_cleanup()");
    
    /* Destroy the device stats list: */
    IBDevicePortsDestroy();
    
    debug_msg("[ibcounters] exiting ibcounters_metric_cleanup()");
}

/*!
    @function ibcounters_metric_handler
    
    Ganglia metric callback function that reports the value of
    the requested metric (by metricIdx at registration) back to
    gmond.
*/
static g_val_t
ibcounters_metric_handler(
  int       metricIdx
)
{
    g_val_t         result;
    IBDevicePort    *p = IBDevicePortsHead;
    int             modIdx = metricIdx;
    
    debug_msg("[ibcounters] entered ibcounters_metric_handler(%d)", metricIdx);
    IBDevicePortsReadCounters();
    
    while ( p && (modIdx >= kIBMaxCounterIdx) ) {
        p = p->link;
        modIdx -= kIBMaxCounterIdx;
    }
    if ( p && (p->fields[modIdx].fieldState == kIBFieldStateValued) ) {
        result.d = p->fields[modIdx].currentValue;
        debug_msg("[ibcounters]  REPORTED %s -> %g", ibcounters_module.metrics_info[metricIdx].name, result.d);
    } else {
        result.d = 0;
    }
    debug_msg("[ibcounters] exiting ibcounters_metric_handler(%d)", metricIdx);
    return result;
}

/*!
    @constant ibcounters_module
    
    Ganglia module definition for the ibcounters functionality implemented above.
*/
mmodule ibcounters_module =
{
    STD_MMODULE_STUFF,
    ibcounters_metric_init,
    ibcounters_metric_cleanup,
    NULL, /* Dynamically created */
    ibcounters_metric_handler,
};
