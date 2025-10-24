/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSCORD_INFERENCE_STREAM_C_API_PROPERTY_C_TYPES_H_
#define SENSCORD_INFERENCE_STREAM_C_API_PROPERTY_C_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#include "senscord/c_api/property_c_types.h"
#include "senscord/config.h"

/**
 * @brief Stream types
 * @see senscord::kStreamTypeInference.
 */
#define SENSCORD_STREAM_TYPE_INFERENCE_STREAM "inference"

/**
 * @brief Basic raw data type names.
 * @see senscord::kRawDataTypeInference.
 */
#define SENSCORD_RAW_DATA_TYPE_INFERENCE "inference_data"

/**
 * @brief Inference data formats
 */
/* User defined */
#define SENSCORD_INFERENCE_DATA_FORMAT_USER_DEFINED "inference_user_defined"

/* Tensor of floats */
#define SENSCORD_INFERENCE_DATA_FORMAT_TENSOR_32F "inference_t32f"

enum senscord_camera_scaling_policy_t {
  SENSCORD_CAMERA_SCALING_POLICY_AUTO,
  SENSCORD_CAMERA_SCALING_POLICY_SENSITIVITY,
  SENSCORD_CAMERA_SCALING_POLICY_RESOLUTION,
};

enum senscord_camera_exposure_mode_t {
  SENSCORD_CAMERA_EXPOSURE_MODE_AUTO,
  SENSCORD_CAMERA_EXPOSURE_MODE_GAIN_FIX,
  SENSCORD_CAMERA_EXPOSURE_MODE_TIME_FIX,
  SENSCORD_CAMERA_EXPOSURE_MODE_MANUAL,
  SENSCORD_CAMERA_EXPOSURE_MODE_HOLD,
};

enum senscord_rotation_angle_t {
  SENSCORD_ROTATION_ANGLE_0_DEG,
  SENSCORD_ROTATION_ANGLE_90_DEG,
  SENSCORD_ROTATION_ANGLE_180_DEG,
  SENSCORD_ROTATION_ANGLE_270_DEG,
};

enum senscord_camera_anti_flicker_mode {
  SENSCORD_CAMERA_ANTI_FLICKER_MODE_OFF,
  SENSCORD_CAMERA_ANTI_FLICKER_MODE_AUTO,
  SENSCORD_CAMERA_ANTI_FLICKER_MODE_FORCE_50HZ,
  SENSCORD_CAMERA_ANTI_FLICKER_MODE_FORCE_60HZ,
};

enum senscord_inference_white_balance_mode_t {
  SENSCORD_INFERENCE_WHITE_BALANCE_MODE_AUTO,
  SENSCORD_INFERENCE_WHITE_BALANCE_MODE_MANUAL_PRESET,
  SENSCORD_INFERENCE_WHITE_BALANCE_MODE_MANUAL_GAIN,
  SENSCORD_INFERENCE_WHITE_BALANCE_MODE_HOLD,
};

enum senscord_camera_auto_exposure_metering_mode_t {
  SENSCORD_CAMERA_AUTO_EXPOSURE_METERING_MODE_FULL_SCREEN,
  SENSCORD_CAMERA_AUTO_EXPOSURE_METERING_MODE_USER_WINDOW,
};

enum senscord_auto_white_balance_metering_mode_t {
  SENSCORD_AUTO_WHITE_BALANCE_METERING_MODE_FULL_SCREEN,
  SENSCORD_AUTO_WHITE_BALANCE_METERING_MODE_USER_WINDOW,
};

/**
 * ImageRotationProperty
 * @see senscord::kImageRotationPropertyKey
 */
#define SENSCORD_IMAGE_ROTATION_PROPERTY_KEY "image_rotation_property"

/**
 * ImageRotationProperty
 * @see senscord::kImageRotationProperty
 */
struct senscord_image_rotation_property_t {
  enum senscord_rotation_angle_t rotation_angle;
};

/**
 * InferenceProperty
 * @see senscord::kInferencePropertyKey
 */
#define SENSCORD_INFERENCE_PROPERTY_KEY "inference_property"

/** Length of the inference data type string. */
#define SENSCORD_INFERENCE_DATA_TYPE_LENGTH 64

/**
 * InferenceProperty
 * @see senscord::kInferencePropertyKey
 */
struct senscord_inference_property_t {
  char data_type[SENSCORD_INFERENCE_DATA_TYPE_LENGTH];
};

/**
 * TensorSharpesProperty
 * @see senscord::kTensorSharpesPropertyKey
 */
#define SENSCORD_TENSOR_SHAPES_PROPERTY_KEY "tensor_shapes_property"

/** Length of the shapes array string. */
#define SENSCORD_SHAPES_ARRAY_LENGTH 192

/**
 * TensorSharpesProperty
 * @see senscord::kTensorSharpesPropertyKey
 */
struct senscord_tensor_shapes_property_t {
  uint32_t tensor_count;
  uint32_t shapes_array[SENSCORD_SHAPES_ARRAY_LENGTH];
};

/* AIModelBundleIdProperty
 * @see senscord::kAIModelBundleIdPropertyKey
 */
#define SENSCORD_AI_MODEL_BUNDLE_ID_PROPERTY_KEY "ai_model_bundle_id_property"

/** Max buffer size of the AI model bundle id string including null terminate */
#define SENSCORD_AI_MODEL_BUNDLE_ID_LENGTH 128

struct senscord_ai_model_bundle_id_property_t {
  char ai_model_bundle_id[SENSCORD_AI_MODEL_BUNDLE_ID_LENGTH];
};

/**
 * AIModelIndexProperty
 * @see senscord::kAIModelIndexProperty
 */
#define SENSCORD_AI_MODEL_INDEX_PROPERTY_KEY "ai_model_index_property"

/**
 * AIModelIndexProperty
 * @see senscord::kAIModelIndexProperty
 */
struct senscord_ai_model_index_property_t {
  uint32_t ai_model_index;
};

/**
 * PostProcessAvailableProperty
 * @see senscord::kPostProcessAvailableProperty
 */
#define SENSCORD_POST_PROCESS_AVAILABLE_PROPERTY_KEY \
  "post_process_available_property"

/**
 * PostProcessAvailableProperty
 * @see senscord::kPostProcessAvailableProperty
 */
struct senscord_post_process_available_property_t {
  union {
    bool is_available;

    /* TODO: remove this typo work around. */
    bool is_aveilable;
  };
};

/**
 * PostProcessParameterProperty
 * @see senscord::kPostProcessParameterProperty
 */
#define SENSCORD_POST_PROCESS_PARAMETER_PROPERTY_KEY \
  "post_process_parameter_property"

#define SENSCORD_INFERENCE_POST_PROCESS_PARAM_SIZE 256

/**
 * PostProcessParameterProperty
 * @see senscord::kPostProcessParameterProperty
 */
struct senscord_post_process_parameter_property_t {
  uint8_t param[SENSCORD_INFERENCE_POST_PROCESS_PARAM_SIZE];
};

/**
 * CameraFrameRateProperty
 * @see senscord::kCameraFrameRatePropertyKey
 */
#define SENSCORD_CAMERA_FRAME_RATE_PROPERTY_KEY "camera_frame_rate_property"

/**
 * CameraFrameRateProperty
 * @see senscord::kCameraFrameRateProperty
 */
struct senscord_camera_frame_rate_property_t {
  uint32_t num;
  uint32_t denom;
};

/**
 * CameraAutoExposureProperty
 * @see senscord::kCameraAutoExposurePropertyKey
 */
#define SENSCORD_CAMERA_AUTO_EXPOSURE_PROPERTY_KEY \
  "camera_auto_exposure_property"

/**
 * CameraAutoExposureProperty
 * @see senscord::kCameraAutoExposurePropertyKey
 */
struct senscord_camera_auto_exposure_property_t {
  uint32_t max_exposure_time;
  uint32_t min_exposure_time;
  float max_gain;
  uint32_t convergence_speed;
};

/**
 * CameraEvCompensationProperty
 * @see senscord::kCameraEvCompensationProperty
 */
#define SENSCORD_CAMERA_EV_COMPENSATION_PROPERTY_KEY \
  "camera_ev_compensation_property"

/**
 * CameraEvCompensationProperty
 * @see senscord::kCameraEvCompensationProperty
 */
struct senscord_camera_ev_compensation_property_t {
  float ev_compensation;
};

/**
 * CameraAntiFlickerModeProperty
 * @see senscord::kCameraAntiFlickerModeProperty
 */
#define SENSCORD_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY \
  "camera_anti_flicker_mode_property"

/* TODO: remove this typo work around. */
#define SENSCORD_CAMERA_ANTI_FLICKER_PROPERTY_KEY \
  SENSCORD_CAMERA_ANTI_FLICKER_MODE_PROPERTY_KEY

/**
 * CameraAntiFlickerModeProperty
 * @see senscord::kCameraAntiFlickerModeProperty
 */
struct senscord_camera_anti_flicker_mode_property_t {
  enum senscord_camera_anti_flicker_mode anti_flicker_mode;
};

/**
 * CameraImageSizeProperty
 * @see senscord::kCameraImageSizePropertyKey
 */
#define SENSCORD_CAMERA_IMAGE_SIZE_PROPERTY_KEY "camera_image_size_property"

/**
 * CameraImageSizeProperty
 * @see senscord::CameraImageSizeProperty
 */
struct senscord_camera_image_size_property_t {
  uint32_t width;
  uint32_t height;
  enum senscord_camera_scaling_policy_t scaling_policy;
};

/**
 * CameraImageFlipProperty
 * @see senscord::kCameraImageFlipPropertyKey
 */
#define SENSCORD_CAMERA_IMAGE_FLIP_PROPERTY_KEY "camera_image_flip_property"

/**
 * CameraImageFlipProperty
 * @see senscord::CameraImageFlipProperty
 */
struct senscord_camera_image_flip_property_t {
  bool flip_horizontal;
  bool flip_vertical;
};

/**
 * CameraDigitalZoomProperty
 * @see senscord::kCameraDigitalZoomPropertyKey
 */
#define SENSCORD_CAMERA_DIGITAL_ZOOM_PROPERTY_KEY "camera_digital_zoom_property"

/**
 * CameraDigitalZoomProperty
 * @see senscord::CameraDigitalZoomProperty
 */
struct senscord_camera_digital_zoom_property_t {
  float magnification;
};

/**
 * CameraExposureModeProperty
 * @see senscord::kCameraExposureModePropertyKey
 */
#define SENSCORD_CAMERA_EXPOSURE_MODE_PROPERTY_KEY \
  "camera_exposure_mode_property"

/**
 * CameraExposureModeProperty
 * @see senscord::kCameraExposureModePropertyKey
 */
struct senscord_camera_exposure_mode_property_t {
  enum senscord_camera_exposure_mode_t mode;
};

/**
 * CameraManualExposureProperty
 * @see senscord::kCameraManualExposurePropertyKey
 */
#define SENSCORD_CAMERA_MANUAL_EXPOSURE_PROPERTY_KEY \
  "camera_manual_exposure_property"

/**
 * CameraManualExposureProperty
 * @see senscord::kCameraManualExposurePropertyKey
 */
struct senscord_camera_manual_exposure_property_t {
  uint32_t exposure_time;
  float gain;
};

/**
 * WhiteBalanceModeProeperty
 * @see senscord::kWhiteBalanceModeProeperty
 */
#define SENSCORD_WHITE_BALANCE_MODE_PROPERTY_KEY "white_balance_mode_property"

/**
 * WhiteBalanceModeProeperty
 * @see senscord::kWhiteBalanceModeProeperty
 */
struct senscord_white_balance_mode_property_t {
  enum senscord_inference_white_balance_mode_t mode;
};

/**
 * AutoWhiteBalaneProperty
 * @see senscord::kAutoWhiteBalaneProperty
 */
#define SENSCORD_AUTO_WHITE_BALANCE_PROPERTY_KEY "auto_white_balance_property"

/**
 * AutoWhiteBalaneProperty
 * @see senscord::kAutoWhiteBalaneProperty
 */
struct senscord_auto_white_balance_property_t {
  uint32_t convergence_speed;
};

/**
 * ManualWhiteBalancePresetProperty
 * @see senscord::kManualWhiteBalancePresetPropertyKey
 */
#define SENSCORD_MANUAL_WHITE_BALANCE_PRESET_PROPERTY_KEY \
  "manual_white_balance_preset_property"

/**
 * ManualWhiteBalancePresetProperty
 * @see senscord::kManualWhiteBalancePresetPropertyKey
 */
struct senscord_manual_white_balance_preset_property_t {
  uint32_t color_temperature;
};

/**
 * ManualWhiteBalanceGainProperty
 * @see senscord::kManualWhiteBalanceGainPropertyKey
 */
#define SENSCORD_MANUAL_WHITE_BALANCE_GAIN_PROPERTY_KEY \
  "manual_white_balance_gain_property"

/**
 * ManualWhiteBalanceGainProperty
 * @see senscord::kManualWhiteBalanceGainPropertyKey
 */
struct senscord_white_balance_gains_t {
  float red;
  float blue;
};

struct senscord_manual_white_balance_gain_property_t {
  struct senscord_white_balance_gains_t gains;
};

/**
 * InfoStringProperty
 * @see senscord::kInfoStringPropertyKey
 */
#define SENSCORD_INFO_STRING_PROPERTY_KEY "info_string_property"

/** Length of the info string. */
#define SENSCORD_INFO_STRING_LENGTH 128

/** Setting the category that member of InfoStringProperty */
/* for AITRIOS hardware info */
#define SENSCORD_INFO_STRING_SENSOR_NAME    0x00000000
#define SENSCORD_INFO_STRING_SENSOR_ID      0x00000001
#define SENSCORD_INFO_STRING_KEY_GENERATION 0x00000002
/* for AITRIOS sensor info */
#define SENSCORD_INFO_STRING_FIRMWARE_VERSION 0x00010000
#define SENSCORD_INFO_STRING_LOADER_VERSION   0x00010001
#define SENSCORD_INFO_STRING_AI_MODEL_VERSION 0x00010002
/* for vendor extension */
#define SENSCORD_INFO_STRING_VENDOR_BASE            0x80000000
#define SENSCORD_INFO_STRING_AIISP_DEVICE_ID        0x80000101
#define SENSCORD_INFO_STRING_AIISP_FIRMWARE_VERSION 0x80000102
#define SENSCORD_INFO_STRING_AIISP_LOADER_VERSION   0x80000103
#define SENSCORD_INFO_STRING_AIISP_AI_MODEL_VERSION 0x80000104

/**
 * InfoStringProperty
 * @see senscord::kInfoStringPropertyKey
 */
struct senscord_info_string_property_t {
  uint32_t category;
  char info[SENSCORD_INFO_STRING_LENGTH];
};

/**
 * @brief Temperature enable information.
 * @see senscord::TemperatureEnable
 */
struct senscord_temperature_enable_t {
  /** Sensor ID. */
  uint32_t sensor_id;
  /** Temperature data. */
  bool enable;
};

/**
 * TemperatureEnableProperty
 * @see senscord::kTemperatureEnablePropertyKey
 */
#define SENSCORD_TEMPERATURE_ENABLE_PROPERTY_KEY "temperature_enable_property"

/**
 * @brief Property for the temperature.
 * @see senscord::TemperatureProperty
 */
struct senscord_temperature_enable_property_t {
  /** Count of the array. */
  uint32_t count;
  /** Array of availablility for each temperature sensor. */
  struct senscord_temperature_enable_t
      temperatures[SENSCORD_TEMPERATURE_LIST_MAX];
};

/**
 * InputDataTypeProperty
 * @see senscord::kInputDataTypePropertyKey
 */
#define SENSCORD_INPUT_DATA_TYPE_PROPERTY_KEY "input_data_type_property"

/**
 * @brief Property for input data type.
 * @see senscord::InputDataTypeProperty
 */
struct senscord_input_data_type_property_t {
  /** Count of the array. */
  uint32_t count;
  /** Array of enabled channl. */
  uint32_t channels[SENSCORD_CHANNEL_LIST_MAX];
};

/**
 * CameraAutoExposureMeteringProperty
 * @see senscord::kCameraAutoExposureMeteringPropertyKey
 */
#define SENSCORD_CAMERA_AUTO_EXPOSURE_METERING_PROPERTY_KEY \
  "camera_auto_exposure_metering_property"

/**
 * CameraAutoExposureMeteringProperty
 * @see senscord::CameraAutoExposureMeteringProperty
 */
struct senscord_camera_auto_exposure_metering_property_t {
  enum senscord_camera_auto_exposure_metering_mode_t mode;
  struct senscord_rectangle_region_parameter_t window;
};

/**
 * AutoWhiteBalanceMeteringProperty
 * @see senscord::kAutoWhiteBalanceMeteringPropertyKey
 */
#define SENSCORD_AUTO_WHITE_BALANCE_METERING_PROPERTY_KEY \
  "auto_white_balance_metering_property"

/**
 * AutoWhiteBalanceMeteringProperty
 * @see senscord::AutoWhiteBalaneProperty
 */
struct senscord_auto_white_balance_metering_property_t {
  enum senscord_auto_white_balance_metering_mode_t mode;
  struct senscord_rectangle_region_parameter_t window;
};

/**
 * SubFrameProperty
 * @see senscord::kSubFramePropertyKey
 */
#define SENSCORD_SUB_FRAME_PROPERTY_KEY "sub_frame_property"

/**
 * @brief Property for sub frame information.
 * @see senscord::SubFrameProperty
 */
struct senscord_sub_frame_property_t {
  uint32_t current_num;
  uint32_t division_num;
};

/**
 * AIDeviceSequenceNumberProperty
 * @see senscord::kAIDeviceSequenceNumberPropertyKey
 */
#define SENSCORD_AI_DEVICE_SEQUENCE_NUMBER_PROPERTY_KEY \
  "ai_device_sequence_number_property"

/**
 * @brief Property for sequence number of AI-Device
 * @see senscord::AIDeviceSequenceNumberProperty
 */
struct senscord_ai_device_sequence_number_property_t {
  /** The sequence number of frame output by AI-Device */
  uint64_t device_sequence_number;
};

/**
 * IspImageProperty
 * @see senscord::kIspImagePropertyKey
 */
#define SENSCORD_ISP_IMAGE_PROPERTY_KEY "isp_image_property"

/**
 * @brief Property for raw image data.
 * @see senscord::IspImageProperty
 */
struct senscord_isp_image_property_t {
  uint32_t width;        /**< Image width. */
  uint32_t height;       /**< Image height. */
  uint32_t stride_bytes; /**< Image stride. */
  /** The format of a pixel. */
  char pixel_format[SENSCORD_PIXEL_FORMAT_LENGTH];
};

/**
 * IspFrameRateProperty
 * @see senscord::kIspFrameRatePropertyKey
 */
#define SENSCORD_ISP_FRAME_RATE_PROPERTY_KEY "isp_frame_rate_property"

/**
 * IspFrameRateProperty
 * @see senscord::IspFrameRateProperty
 */
struct senscord_isp_frame_rate_property_t {
  uint32_t num;   /**< Numerator of the frame rate. */
  uint32_t denom; /**< Denominator of the frame rate. */
};

#endif /*  SENSCORD_INFERENCE_STREAM_C_API_PROPERTY_C_TYPES_H_ */
