#include "sn74hc595i.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sn74hc595i {

static const char *const TAG = "sn74hc595i";

void SN74HC595IComponent::pre_setup_() {
  ESP_LOGCONFIG(TAG, "Setting up SN74HC595I...");

  if (this->have_oe_pin_) {  // disable output
    this->oe_pin_->setup();
    this->oe_pin_->digital_write(true);
  }
}
void SN74HC595IComponent::post_setup_() {
  this->latch_pin_->setup();
  this->latch_pin_->digital_write(false);

  // send state to shift register
  this->write_gpio();
}

void SN74HC595IGPIOComponent::setup() {
  this->pre_setup_();
  this->clock_pin_->setup();
  this->data_pin_->setup();
  this->clock_pin_->digital_write(false);
  this->data_pin_->digital_write(false);
  this->post_setup_();
}

#ifdef USE_SPI
void SN74HC595ISPIComponent::setup() {
  this->pre_setup_();
  this->spi_setup();
  this->post_setup_();
}
#endif

void SN74HC595IComponent::dump_config() { ESP_LOGCONFIG(TAG, "SN74HC595I:"); }

void SN74HC595IComponent::digital_write_(uint16_t pin, bool value) {
  if (pin >= this->sr_count_ * 8) {
    ESP_LOGE(TAG, "Pin %u is out of range! Maximum pin number with %u chips in series is %u", pin, this->sr_count_,
             (this->sr_count_ * 8) - 1);
    return;
  }
  if (value) {
    this->value_bytes_[pin / 8] |= (1 << (pin % 8));
  } else {
    this->value_bytes_[pin / 8] &= ~(1 << (pin % 8));
  }
  this->write_gpio();
}
void SN74HC595IComponent::set_inverted_(uint16_t pin, bool inverted) {
  if (pin >= this->sr_count_ * 8) {
    ESP_LOGE(TAG, "Pin %u is out of range! Maximum pin number with %u chips in series is %u", pin, this->sr_count_,
             (this->sr_count_ * 8) - 1);
    return;
  }
  if (inverted) {
    this->inverted_bytes_[pin / 8] |= (1 << (pin % 8));
  } else {
    this->inverted_bytes_[pin / 8] &= ~(1 << (pin % 8));
  }
}

void SN74HC595IGPIOComponent::write_gpio() {
  auto value = this->value_bytes_.rbegin();
  auto inverted = this->inverted_bytes_.rbegin();
  while (value != this->value_bytes_.rend() && inverted != this->inverted_bytes_.rend()) {
    for (int8_t i = 7; i >= 0; i--) {
      bool value_bit = (*value >> i) & 1;
      bool value_inverted = (*inverted >> i) & 1;
      this->data_pin_->digital_write(value_bit != value_inverted);
      this->clock_pin_->digital_write(true);
      this->clock_pin_->digital_write(false);
    }
    value++;
    inverted++;
  }
  SN74HC595IComponent::write_gpio();
}

#ifdef USE_SPI
void SN74HC595ISPIComponent::write_gpio() {
  auto value = this->value_bytes_.rbegin();
  auto inverted = this->inverted_bytes_.rbegin();
  while (value != this->value_bytes_.rend() && inverted != this->inverted_bytes_.rend()) {
    this->enable();
    this->transfer_byte((*byte) ^ (*inverted));
    this->disable();
    value++;
    inverted++;
  }
  SN74HC595IComponent::write_gpio();
}
#endif

void SN74HC595IComponent::write_gpio() {
  // pulse latch to activate new values
  this->latch_pin_->digital_write(true);
  this->latch_pin_->digital_write(false);

  // enable output if configured
  if (this->have_oe_pin_) {
    this->oe_pin_->digital_write(false);
  }
}

float SN74HC595IComponent::get_setup_priority() const { return setup_priority::IO; }

void SN74HC595IGPIOPin::digital_write(bool value) { this->parent_->digital_write_(this->pin_, value); }
void SN74HC595IGPIOPin::set_inverted(bool inverted) { this->parent_->set_inverted_(this->pin_, inverted); }
std::string SN74HC595IGPIOPin::dump_summary() const { return str_snprintf("%u via SN74HC595I", 18, pin_); }

}  // namespace sn74hc595i
}  // namespace esphome