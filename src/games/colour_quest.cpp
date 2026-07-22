#include "games/colour_quest.h"

#include "config.h"
#include "led_manager.h"

void ColourQuestGame::start(uint32_t now) {
  stage_ = Stage::Gate;
  gate_.start(now);
}

void ColourQuestGame::update(uint32_t now) {
  if (stage_ == Stage::Gate) {
    gate_.update(now);
    if (gate_.score() >= Config::COLOUR_GATE_STAGE_SCORE) {
      stage_ = Stage::Transition;
      transitionStartedAt_ = now;
    }
    return;
  }

  if (stage_ == Stage::Transition) {
    if (static_cast<uint32_t>(now - transitionStartedAt_) >=
        Config::COLOUR_QUEST_TRANSITION_MS) {
      stage_ = Stage::Boss;
      boss_.start(now);
    }
    return;
  }

  boss_.update(now);
}

void ColourQuestGame::render(uint32_t now) const {
  if (stage_ == Stage::Gate) {
    gate_.render(now);
    return;
  }
  if (stage_ == Stage::Boss) {
    boss_.render(now);
    return;
  }

  LedManager::clearStrip();
  uint16_t count = static_cast<uint16_t>(
      (static_cast<uint32_t>(now - transitionStartedAt_) * Config::LED_COUNT) /
      Config::COLOUR_QUEST_TRANSITION_MS);
  if (count > Config::LED_COUNT) {
    count = Config::LED_COUNT;
  }
  LedManager::setStripRange(0, count, Config::COLOUR_GATE_SUCCESS_COLOR);
}

uint16_t ColourQuestGame::score() const {
  if (stage_ == Stage::Boss) {
    const uint16_t total = Config::COLOUR_GATE_STAGE_SCORE + boss_.score();
    return total > 999U ? 999U : total;
  }
  return gate_.score();
}
