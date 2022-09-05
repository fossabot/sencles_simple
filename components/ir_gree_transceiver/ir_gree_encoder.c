/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "string.h"
#include "ir_gree_encoder.h"

static const char *TAG = "gree_encoder";

#define GETBITS8(data, offset, size) \
    (((data) & (((uint8_t)UINT8_MAX >> (8 - (size))) << (offset))) >> (offset))

typedef struct
{
    rmt_encoder_t base;                    // the base "class", declares the standard encoder interface
    rmt_encoder_t *copy_encoder;           // use the copy_encoder to encode the leading and ending pulse
    rmt_encoder_t *bytes_encoder;          // use the bytes_encoder to encode the address and command data
    rmt_symbol_word_t gree_leading_symbol; // GREE leading code with RMT representation
    rmt_symbol_word_t gree_connect_symbol; // GREE ending code with RMT representation
    rmt_symbol_word_t gree_lconnect_symbol;
    rmt_symbol_word_t gree_gap_symbol;

    rmt_symbol_word_t gree_low_level;
    rmt_symbol_word_t gree_high_level;
    int state;
} rmt_ir_gree_encoder_t;

static size_t rmt_encode_ir_gree(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;
    GreeProtocol_t *scan_code = (GreeProtocol_t *)primary_data;
    rmt_encoder_handle_t copy_encoder = gree_encoder->copy_encoder;
    rmt_encoder_handle_t bytes_encoder = gree_encoder->bytes_encoder;

    uint8_t i = 0;

    switch (gree_encoder->state)
    {
    case 0: // send leading code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_leading_symbol,
                                                sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 1; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 1: // send data four bytes at a time
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[0], 4 * sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 2; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 2: // send identifier 0b010
    for
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_low_level, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 3;
            //i++;
        }
        //if (session_state & RMT_ENCODING_COMPLETE & (i == 2))
        //{
            //i = 0;
            //gree_encoder->state = 3; // we can only switch to next state when current encoder finished
        //}
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }

    // fall-through
    case 3: // send connect code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_connect_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 4;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 4: // send second four bytes
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, &scan_code->raw[4], 4 * sizeof(uint8_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 5; // back to the initial encoding session
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 5: // send long connect code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gree_encoder->gree_lconnect_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            gree_encoder->state = 0;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
        // fall-through
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_ir_gree_encoder(rmt_encoder_t *encoder)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_del_encoder(gree_encoder->copy_encoder);
    rmt_del_encoder(gree_encoder->bytes_encoder);
    free(gree_encoder);
    return ESP_OK;
}

static esp_err_t rmt_ir_gree_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_ir_gree_encoder_t *gree_encoder = __containerof(encoder, rmt_ir_gree_encoder_t, base);
    rmt_encoder_reset(gree_encoder->copy_encoder);
    rmt_encoder_reset(gree_encoder->bytes_encoder);
    gree_encoder->state = 0;
    return ESP_OK;
}

esp_err_t rmt_new_ir_gree_encoder(const ir_gree_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_ir_gree_encoder_t *gree_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gree_encoder = calloc(1, sizeof(rmt_ir_gree_encoder_t));
    ESP_GOTO_ON_FALSE(gree_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for ir gree encoder");
    gree_encoder->base.encode = rmt_encode_ir_gree;
    gree_encoder->base.del = rmt_del_ir_gree_encoder;
    gree_encoder->base.reset = rmt_ir_gree_encoder_reset;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &gree_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    // construct the leading code and ending code with RMT symbol format
    gree_encoder->gree_leading_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 4500ULL * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 9000ULL * config->resolution / 1000000,
    };
    gree_encoder->gree_connect_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 19975 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 680 * config->resolution / 1000000,
    };
    gree_encoder->gree_lconnect_symbol = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 19975 * 2 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 680 * config->resolution / 1000000,
    };
    gree_encoder->gree_low_level = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 510 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 680 * config->resolution / 1000000,
    };
    gree_encoder->gree_high_level = (rmt_symbol_word_t){
        .level0 = 1,
        .duration0 = 1530 * config->resolution / 1000000,
        .level1 = 0,
        .duration1 = 680 * config->resolution / 1000000,
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 510 * config->resolution / 1000000, // T0H=510us
            .level1 = 0,
            .duration1 = 680 * config->resolution / 1000000, // T0L=680us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 1530 * config->resolution / 1000000, // T1H=1530us
            .level1 = 0,
            .duration1 = 680 * config->resolution / 1000000, // T1L=680us
        },
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &gree_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");

    *ret_encoder = &gree_encoder->base;
    return ESP_OK;
err:
    if (gree_encoder)
    {
        if (gree_encoder->bytes_encoder)
        {
            rmt_del_encoder(gree_encoder->bytes_encoder);
        }
        if (gree_encoder->copy_encoder)
        {
            rmt_del_encoder(gree_encoder->copy_encoder);
        }
        free(gree_encoder);
    }
    return ret;
}

/// Reset the internals of the object to a known good state.
void stateReset(void)
{
  for (uint8_t i = 0; i < 16; i++)
    _.raw[i] = 0x0;
  _.raw[3] = 0x50;
  _.raw[11] = 0x70;
}

/// Set up hardware to be able to send a message.
void begin(void) { _irsend.begin(); }

/// Fix up any odd conditions for the current state.
void fixup(void)
{
  // X-Fan mode is only valid in COOL or DRY modes.
  if (_.Mode != kKelvinatorCool && _.Mode != kKelvinatorDry)
    setXFan(false);
  // Duplicate to the 2nd command chunk.
  _.raw[8] = _.raw[0];
  _.raw[9] = _.raw[1];
  _.raw[10] = _.raw[2];
  checksum(); // Calculate the checksums
}

/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void send(const uint16_t repeat) {
  _irsend.sendKelvinator(getRaw(), kKelvinatorStateLength, repeat);
}


/// Get the raw state of the object, suitable to be sent with the appropriate
/// IRsend object method.
/// @return A PTR to the internal state.
uint8_t *getRaw(void) {
  fixup();  // Ensure correct settings before sending.
  return _.raw;
}

/// Set the raw state of the object.
/// @param[in] new_code The raw state from the native IR message.
void setRaw(const uint8_t new_code[]) {
  memcpy(_.raw, new_code, kKelvinatorStateLength);
}

/// Calculate the checksum for a given block of state.
/// @param[in] block A pointer to a block to calc the checksum of.
/// @param[in] length Length of the block array to checksum.
/// @return The calculated checksum value.
/// @note Many Bothans died to bring us this information.
uint8_t calcBlockChecksum(const uint8_t *block,
                                          const uint16_t length) {
  uint8_t sum = 10;
  // Sum the lower half of the first 4 bytes of this block.
  for (uint8_t i = 0; i < 4 && i < length - 1; i++, block++)
    sum += (*block & 0b1111);
  // then sum the upper half of the next 3 bytes.
  for (uint8_t i = 4; i < length - 1; i++, block++) sum += (*block >> 4);
  // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
  return sum & 0b1111;
}

/// Calculate the checksum for the internal state.
void checksum(void) {
  _.Sum1 = calcBlockChecksum(_.raw);
  _.Sum2 = calcBlockChecksum(_.raw + 8);
}

/// Verify the checksum is valid for a given state.
/// @param[in] state The array to verify the checksum of.
/// @param[in] length The size of the state.
/// @return A boolean indicating if it is valid.
bool validChecksum(const uint8_t state[],
                                   const uint16_t length) {
  for (uint16_t offset = 0; offset + 7 < length; offset += 8) {
    // Top 4 bits of the last byte in the block is the block's checksum.
    if (GETBITS8(state[offset + 7], 4, 4) !=
        calcBlockChecksum(state + offset))
      return false;
  }
  return true;
}

/// Set the internal state to have the power on.
void on(void) { setPower(true); }

/// Set the internal state to have the power off.
void off(void) {setPower(false); }

/// Set the internal state to have the desired power.
/// @param[in] on The desired power state.
void setPower(const bool on) {
  _.Power = on;
}

/// Get the power setting from the internal state.
/// @return A boolean indicating if the power setting.
bool getPower(void) const {
  return _.Power;
}

/// Set the temperature setting.
/// @param[in] degrees The temperature in degrees celsius.
void setTemp(const uint8_t degrees) {
  uint8_t temp = std::max(kKelvinatorMinTemp, degrees);
  temp = std::min(kKelvinatorMaxTemp, temp);
  _.Temp = temp - kKelvinatorMinTemp;
}

/// Get the current temperature setting.
/// @return Get current setting for temp. in degrees celsius.
uint8_t getTemp(void) const {
  return _.Temp + kKelvinatorMinTemp;
}

/// Set the speed of the fan.
/// @param[in] speed 0 is auto, 1-5 is the speed
void setFan(const uint8_t speed) {
  uint8_t fan = min(kKelvinatorFanMax, speed);  // Bounds check

  // Only change things if we need to.
  if (fan != _.Fan) {
    // Set the basic fan values.
    _.BasicFan = min(kKelvinatorBasicFanMax, fan);
    // Set the advanced(?) fan value.
    _.Fan = fan;
    // Turbo mode is turned off if we change the fan settings.
    setTurbo(false);
  }
}

/// Get the current fan speed setting.
/// @return The current fan speed.
uint8_t getFan(void) const {
  return _.Fan;
}

/// Get the current operation mode setting.
/// @return The current operation mode.
uint8_t getMode(void) const {
  return _.Mode;
}

/// Set the desired operation mode.
/// @param[in] mode The desired operation mode.
void setMode(const uint8_t mode) {
  switch (mode) {
    case kKelvinatorAuto:
    case kKelvinatorDry:
      // When the remote is set to Auto or Dry, it defaults to 25C and doesn't
      // show it.
      setTemp(kKelvinatorAutoTemp);
      // FALL-THRU
    case kKelvinatorHeat:
    case kKelvinatorCool:
    case kKelvinatorFan:
      _.Mode = mode;
      break;
    default:
      setTemp(kKelvinatorAutoTemp);
      _.Mode = kKelvinatorAuto;
      break;
  }
}

/// Set the Vertical Swing mode of the A/C.
/// @param[in] automatic Do we use the automatic setting?
/// @param[in] position The position/mode to set the vanes to.
void setSwingVertical(const bool automatic,
                                      const uint8_t position) {
  _.SwingAuto = (automatic || _.SwingH);
  uint8_t new_position = position;
  if (!automatic) {
    switch (position) {
      case kKelvinatorSwingVHighest:
      case kKelvinatorSwingVUpperMiddle:
      case kKelvinatorSwingVMiddle:
      case kKelvinatorSwingVLowerMiddle:
      case kKelvinatorSwingVLowest:
        break;
      default:
        new_position = kKelvinatorSwingVOff;
    }
  } else {
    switch (position) {
      case kKelvinatorSwingVAuto:
      case kKelvinatorSwingVLowAuto:
      case kKelvinatorSwingVMiddleAuto:
      case kKelvinatorSwingVHighAuto:
        break;
      default:
        new_position = kKelvinatorSwingVAuto;
    }
  }
  _.SwingV = new_position;
}

/// Get the Vertical Swing Automatic mode setting of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool getSwingVerticalAuto(void) const {
  return _.SwingV & 0b0001;
}

/// Get the Vertical Swing position setting of the A/C.
/// @return The native position/mode.
uint8_t getSwingVerticalPosition(void) const {
  return _.SwingV;
}

/// Control the current horizontal swing setting.
/// @param[in] on The desired setting.
void setSwingHorizontal(const bool on) {
  _.SwingH = on;
  _.SwingAuto = (on || (_.SwingV & 0b0001));
}

/// Is the horizontal swing setting on?
/// @return The current value.
bool getSwingHorizontal(void) const {
  return _.SwingH;
}

/// Control the current Quiet setting.
/// @param[in] on The desired setting.
void setQuiet(const bool on) {
  _.Quiet = on;
}

/// Is the Quiet setting on?
/// @return The current value.
bool getQuiet(void) const {
  return _.Quiet;
}

/// Control the current Ion Filter setting.
/// @param[in] on The desired setting.
void setIonFilter(const bool on) {
  _.IonFilter = on;
}

/// Is the Ion Filter setting on?
/// @return The current value.
bool getIonFilter(void) const {
  return _.IonFilter;
}

/// Control the current Light setting.
/// i.e. The LED display on the A/C unit that shows the basic settings.
/// @param[in] on The desired setting.
void setLight(const bool on) {
  _.Light = on;
}

/// Is the Light (Display) setting on?
/// @return The current value.
bool getLight(void) const {
  return _.Light;
}

/// Control the current XFan setting.
/// This setting will cause the unit blow air after power off to dry out the
/// A/C device.
/// @note XFan mode is only valid in Cool or Dry mode.
/// @param[in] on The desired setting.
void setXFan(const bool on) {
  _.XFan = on;
}

/// Is the XFan setting on?
/// @return The current value.
bool getXFan(void) const {
  return _.XFan;
}

/// Control the current Turbo setting.
/// @note Turbo mode is turned off if the fan speed is changed.
/// @param[in] on The desired setting.
void setTurbo(const bool on) {
  _.Turbo = on;
}

/// Is the Turbo setting on?
/// @return The current value.
bool getTurbo(void) const {
  return _.Turbo;
}

/// Convert a standard A/C mode (opmode_t) into it a native mode.
/// @param[in] mode A opmode_t operation mode.
/// @return The native mode equivalent.
uint8_t convertMode(const opmode_t mode) {
  switch (mode) {
    case opmode_t::kCool: return kKelvinatorCool;
    case opmode_t::kHeat: return kKelvinatorHeat;
    case opmode_t::kDry:  return kKelvinatorDry;
    case opmode_t::kFan:  return kKelvinatorFan;
    default:                     return kKelvinatorAuto;
  }
}

/// Convert a swingv_t enum into it's native setting.
/// @param[in] swingv The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t convertSwingV(const swingv_t swingv) {
  switch (swingv) {
    case swingv_t::kHighest: return kKelvinatorSwingVHighest;
    case swingv_t::kHigh:    return kKelvinatorSwingVHighAuto;
    case swingv_t::kMiddle:  return kKelvinatorSwingVMiddle;
    case swingv_t::kLow:     return kKelvinatorSwingVLowAuto;
    case swingv_t::kLowest:  return kKelvinatorSwingVLowest;
    default:                        return kKelvinatorSwingVAuto;
  }
}

/// Convert a native mode to it's opmode_t equivalent.
/// @param[in] mode A native operating mode value.
/// @return The opmode_t equivalent.
opmode_t toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kKelvinatorCool: return opmode_t::kCool;
    case kKelvinatorHeat: return opmode_t::kHeat;
    case kKelvinatorDry:  return opmode_t::kDry;
    case kKelvinatorFan:  return opmode_t::kFan;
    default:              return opmode_t::kAuto;
  }
}

/// Convert a native fan speed to it's fanspeed_t equivalent.
/// @param[in] speed A native fan speed value.
/// @return The fanspeed_t equivalent.
fanspeed_t toCommonFanSpeed(const uint8_t speed) {
  return (fanspeed_t)speed;
}

/// Convert the internal A/C object state to it's state_t equivalent.
/// @return A state_t containing the current settings.
state_t toCommon(void) const {
  state_t result{};
  result.protocol = decode_type_t::KELVINATOR;
  result.model = -1;  // Unused.
  result.power = _.Power;
  result.mode = toCommonMode(_.Mode);
  result.celsius = true;
  result.degrees = getTemp();
  result.fanspeed = toCommonFanSpeed(_.Fan);
  result.swingv = _.SwingV ? swingv_t::kAuto : swingv_t::kOff;
  result.swingh = _.SwingH ? swingh_t::kAuto : swingh_t::kOff;
  result.quiet = _.Quiet;
  result.turbo = _.Turbo;
  result.light = _.Light;
  result.filter = _.IonFilter;
  result.clean = _.XFan;
  // Not supported.
  result.econo = false;
  result.beep = false;
  result.sleep = -1;
  result.clock = -1;
  return result;
}

/// Convert the internal settings into a human readable string.
/// @return A String.
String toString(void) const {
  String result = "";
  result.reserve(160);  // Reserve some heap for the string to reduce fragging.
  result += addBoolToString(_.Power, kPowerStr, false);
  result += addModeToString(_.Mode, kKelvinatorAuto, kKelvinatorCool,
                            kKelvinatorHeat, kKelvinatorDry, kKelvinatorFan);
  result += addTempToString(getTemp());
  result += addFanToString(_.Fan, kKelvinatorFanMax, kKelvinatorFanMin,
                           kKelvinatorFanAuto, kKelvinatorFanAuto,
                           kKelvinatorBasicFanMax);
  result += addBoolToString(_.Turbo, kTurboStr);
  result += addBoolToString(_.Quiet, kQuietStr);
  result += addBoolToString(_.XFan, kXFanStr);
  result += addBoolToString(_.IonFilter, kIonStr);
  result += addBoolToString(_.Light, kLightStr);
  result += addBoolToString(_.SwingH, kSwingHStr);
  result += addSwingVToString(_.SwingV, kKelvinatorSwingVAuto,
                              kKelvinatorSwingVHighest,
                              kKelvinatorSwingVHighAuto,
                              kKelvinatorSwingVUpperMiddle,
                              kKelvinatorSwingVMiddle,
                              kKelvinatorSwingVLowerMiddle,
                              kKelvinatorSwingVLowAuto,
                              kKelvinatorSwingVLowest,
                              kKelvinatorSwingVOff,
                              kKelvinatorSwingVAuto, kKelvinatorSwingVAuto,
                              kKelvinatorSwingVAuto);
  return result;
}