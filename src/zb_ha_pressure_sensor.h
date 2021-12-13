#ifndef ZB_HA_PRESSURE_SENSOR_H__
#define ZB_HA_PRESSURE_SENSOR_H__

#include "zboss_api.h"
#include "zb_zcl_pressure_measurement.h"

#define ZB_HA_PRESSURE_SENSOR_IN_CLUSTER_NUM 3 /* IN (server) clusters count */
#define ZB_HA_PRESSURE_SENSOR_OUT_CLUSTER_NUM 1 /* OUT (client) clusters count */
#define ZB_HA_DEVICE_VER_PRESSURE_SENSOR 0
#define ZB_HA_PRESSURE_SENSOR_DEVICE_ID 0x0305
#define ZB_HA_PRESSURE_SENSOR_REPORT_ATTR_COUNT 1

#define ZB_HA_DECLARE_PRESSURE_SENSOR_CLUSTER_LIST(                 \
  cluster_list_name,                                            \
  basic_attr_list,                                              \
  identify_attr_list,                                           \
  pres_measure_attr_list)                                       \
  zb_zcl_cluster_desc_t cluster_list_name[] =                   \
  {                                                             \
    ZB_ZCL_CLUSTER_DESC(                                        \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                               \
      ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),     \
      (identify_attr_list),                                     \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
      ZB_ZCL_MANUF_CODE_INVALID                                 \
    ),                                                          \
    ZB_ZCL_CLUSTER_DESC(                                        \
      ZB_ZCL_CLUSTER_ID_BASIC,                                  \
      ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),        \
      (basic_attr_list),                                        \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
      ZB_ZCL_MANUF_CODE_INVALID                                 \
    ),                                                          \
    ZB_ZCL_CLUSTER_DESC(                                        \
      ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,                   \
      ZB_ZCL_ARRAY_SIZE(pres_measure_attr_list, zb_zcl_attr_t), \
      (pres_measure_attr_list),                                 \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
      ZB_ZCL_MANUF_CODE_INVALID                                 \
    ),                                                          \
    ZB_ZCL_CLUSTER_DESC(                                        \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                               \
      0,                                                        \
      NULL,                                                     \
      ZB_ZCL_CLUSTER_CLIENT_ROLE,                               \
      ZB_ZCL_MANUF_CODE_INVALID                                 \
    )                                                           \
}


#define ZB_ZCL_DECLARE_PRESSURE_SENSOR_PRESSURE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =   \
{                                                                             \
  ep_id,                                                                      \
  ZB_AF_HA_PROFILE_ID,                                                        \
  ZB_HA_PRESSURE_SENSOR_DEVICE_ID,                                            \
  ZB_HA_DEVICE_VER_PRESSURE_SENSOR,                                           \
  0,                                                                          \
  in_clust_num,                                                               \
  out_clust_num,                                                              \
  {                                                                           \
    ZB_ZCL_CLUSTER_ID_BASIC,                                                  \
    ZB_ZCL_CLUSTER_ID_IDENTIFY,                                               \
    ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,                                   \
    ZB_ZCL_CLUSTER_ID_IDENTIFY,                                               \
  }                                                                           \
}
// ZB_AF_SIMPLE_DESC_TYPE requires that the type of that simple descriptor structure exists
// In case of using two devices with the same number of clusters out and clusters in
// only one of them can declare the type to, because you can't define the structure twice


#define ZB_HA_DECLARE_PRESSURE_SENSOR_EP(ep_name, ep_id, cluster_list)       \
ZB_ZCL_DECLARE_PRESSURE_SENSOR_PRESSURE_DESC(ep_name,                        \
    ep_id,                                                                   \
    ZB_HA_PRESSURE_SENSOR_IN_CLUSTER_NUM,                                    \
    ZB_HA_PRESSURE_SENSOR_OUT_CLUSTER_NUM);                                  \
ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info## device_ctx_name,         \
                                   ZB_HA_PRESSURE_SENSOR_REPORT_ATTR_COUNT); \
ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id,                                  \
    ZB_AF_HA_PROFILE_ID,                                                     \
    0,                                                                       \
    NULL,                                                                    \
    ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t),                  \
    cluster_list,                                                            \
    (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name,                        \
    ZB_HA_PRESSURE_SENSOR_REPORT_ATTR_COUNT, reporting_info## device_ctx_name, 0, NULL)


typedef struct
{
    zb_int16_t  measure_value;
    zb_int16_t  min_measure_value;
    zb_int16_t  max_measure_value;
    zb_uint16_t tolerance;
} zb_zcl_pressure_measurement_attrs_t;

#endif /* ZB_HA_PRESSURE_SENSOR_H__ */
