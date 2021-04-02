#include "SpoilersHandler.h"

#include <cmath>
#include <iostream>

using std::cout;
using std::endl;

bool SpoilersHandler::getIsInitialized() const {
  return isInitialized;
}

bool SpoilersHandler::getIsArmed() const {
  return isArmed;
}

double SpoilersHandler::getHandlePosition() const {
  return handlePosition;
}

double SpoilersHandler::getSimPosition() const {
  return simPosition;
}

void SpoilersHandler::setInitialPosition(double position) {
  if (isInitialized) {
    return;
  }
  update(isArmed, fmin(1.0, fmax(0.0, position)));
  isInitialized = true;
}

void SpoilersHandler::setSimulationVariables(double simulationTime_new,
                                             bool isAutopilotEngaged_new,
                                             double airspeed_new,
                                             double thrustLeverAngle_1_new,
                                             double thrustLeverAngle_2_new,
                                             double landingGearCompression_1_new,
                                             double landingGearCompression_2_new) {
  update(simulationTime_new, isArmed, handlePosition, isAutopilotEngaged_new, airspeed_new, thrustLeverAngle_1_new, thrustLeverAngle_2_new,
         getGearStrutCompressionFromAnimation(landingGearCompression_1_new),
         getGearStrutCompressionFromAnimation(landingGearCompression_2_new));
}

void SpoilersHandler::onEventSpoilersOn() {
  update(isArmed, POSITION_FULL);
}

void SpoilersHandler::onEventSpoilersOff() {
  update(isArmed, POSITION_RETRACTED);
}

void SpoilersHandler::onEventSpoilersToggle() {
  update(isArmed, handlePosition > 0 ? POSITION_RETRACTED : POSITION_FULL);
}

void SpoilersHandler::onEventSpoilersSet(double value) {
  update(isArmed, fmin(1.0, fmax(0.0, value / 16384.0)));
}

void SpoilersHandler::onEventSpoilersAxisSet(double value) {
  update(isArmed, fmin(1.0, fmax(0.0, 0.5 + (value / 32768.0))));
}

void SpoilersHandler::onEventSpoilersArmOn() {
  update(true, handlePosition);
}

void SpoilersHandler::onEventSpoilersArmOff() {
  update(false, handlePosition);
}

void SpoilersHandler::onEventSpoilersArmToggle() {
  update(!isArmed, handlePosition);
}

void SpoilersHandler::onEventSpoilersArmSet(bool value) {
  update(value, handlePosition);
}

void SpoilersHandler::update(bool isArmed_new, double handlePosition_new) {
  update(simulationTime, isArmed_new, handlePosition_new, isAutopilotEngaged, airspeed, thrustLeverAngle_1, thrustLeverAngle_2,
         landingGearCompression_1, landingGearCompression_2);
}

void SpoilersHandler::update(double simulationTime_new,
                             bool isArmed_new,
                             double handlePosition_new,
                             bool isAutopilotEngaged_new,
                             double airspeed_new,
                             double thrustLeverAngle_1_new,
                             double thrustLeverAngle_2_new,
                             double landingGearCompression_1_new,
                             double landingGearCompression_2_new) {
  // manual deployment -------------------------------------------------------------------------------------------------

  if (isArmed != isArmed_new || handlePosition != handlePosition_new) {
    // ensure ground spoilers are only armed when handle is in retracted position
    isArmed = isArmed_new && (handlePosition_new == POSITION_RETRACTED);
    // remember handle position
    handlePosition = handlePosition_new;
    // set sim position
    if (isAutopilotEngaged_new) {
      simPosition = fmin(POSITION_LIMIT_AUTOPILOT, handlePosition_new);
    } else {
      simPosition = handlePosition_new;
    }
  }

  // autopilot limitation on transition --------------------------------------------------------------------------------

  if (isAutopilotEngaged_new && isAutopilotEngaged != isAutopilotEngaged_new) {
    simPosition = fmin(POSITION_LIMIT_AUTOPILOT, handlePosition_new);
  }

  // store simulation variables ----------------------------------------------------------------------------------------

  simulationTime = simulationTime_new;
  isAutopilotEngaged = isAutopilotEngaged_new;
  airspeed = airspeed_new;
  thrustLeverAngle_1 = thrustLeverAngle_1_new;
  thrustLeverAngle_2 = thrustLeverAngle_2_new;
  landingGearCompression_1 = landingGearCompression_1_new;
  landingGearCompression_2 = landingGearCompression_2_new;

  // conditions --------------------------------------------------------------------------------------------------------

  // determine conditions
  double numberOfMainLandingGearsOnGround = numberOfLandingGearsOnGround(landingGearCompression_1, landingGearCompression_2);
  bool areThrustLeversAtOrBelowIdle = areAtOrBelowIdle(thrustLeverAngle_1, thrustLeverAngle_2);

  // detect landing condition
  if (conditionLanding) {
    if (airspeed < CONDITION_AIRSPEED) {
      conditionLanding = false;
      timeAirborne = 0;
    }
  } else {
    if (timeAirborne == 0.0 && numberOfMainLandingGearsOnGround == 0.0) {
      timeAirborne = simulationTime;
    } else if (getTimeSinceAirborne(simulationTime, timeAirborne) >= MINIMUM_AIRBORNE_TIME) {
      conditionLanding = true;
    }
  }

  // detect take-off condition
  if (numberOfMainLandingGearsOnGround == 2 && airspeed > CONDITION_AIRSPEED) {
    conditionTakeOff = true;
  } else if (airspeed < CONDITION_AIRSPEED || numberOfMainLandingGearsOnGround == 0) {
    conditionTakeOff = false;
  }

  // take-off phase ----------------------------------------------------------------------------------------------------

  if (conditionTakeOff) {
    if ((isArmed && areThrustLeversAtOrBelowIdle) || isAtLeastOneInReverseAndOtherInIdle(thrustLeverAngle_1, thrustLeverAngle_2)) {
      simPosition = POSITION_FULL;
    }
  }

  // landing phase -----------------------------------------------------------------------------------------------------

  if (conditionLanding) {
    // determine conditions
    bool areThrustLeversBelowClimb = areBelowClimb(thrustLeverAngle_1_new, thrustLeverAngle_2_new);
    bool isAtLeastOneThrustLeverInReverseAndOtherBelowMct =
        isAtLeastOneInReverseAndOtherBelowMct(thrustLeverAngle_1_new, thrustLeverAngle_2_new);

    // armed *or* lever *not* retracted
    if (isArmed || handlePosition > POSITION_RETRACTED) {
      if (numberOfMainLandingGearsOnGround == 2) {
        if (areThrustLeversAtOrBelowIdle || isAtLeastOneThrustLeverInReverseAndOtherBelowMct) {
          // full deployment
          simPosition = POSITION_FULL;
        } else if (isArmed && areThrustLeversBelowClimb) {
          // partial deployment
          simPosition = POSITION_PARTIAL;
        }
      } else if (numberOfMainLandingGearsOnGround >= 1 && areThrustLeversAtOrBelowIdle) {
        // partial deployment
        simPosition = fmax(handlePosition, POSITION_PARTIAL);
      }
    }

    // *not* armed *and* lever retracted
    if (!isArmed && handlePosition == POSITION_RETRACTED) {
      if (isAtLeastOneThrustLeverInReverseAndOtherBelowMct) {
        if (numberOfMainLandingGearsOnGround == 2) {
          // full deployment
          simPosition = POSITION_FULL;
        } else if (numberOfMainLandingGearsOnGround >= 1) {
          // partial deployment
          simPosition = POSITION_PARTIAL;
        }
      }
    }

    // on touch & go retract spoilers when at least one thrust lever is > 20°
    if (numberOfMainLandingGearsOnGround > 0 &&
        (thrustLeverAngle_1 > TLA_CONDITION_TOUCH_GO || thrustLeverAngle_2 > TLA_CONDITION_TOUCH_GO)) {
      simPosition = fmax(handlePosition, POSITION_RETRACTED);
    }
  }
}

double SpoilersHandler::getGearStrutCompressionFromAnimation(double animationPosition) {
  return fmin(1.0, fmax(0.0, 2 * (animationPosition - 0.5)));
}

double SpoilersHandler::getTimeSinceAirborne(double simulationTime, double timeAirborne) {
  return timeAirborne > 0 ? simulationTime - timeAirborne : 0;
}

double SpoilersHandler::numberOfLandingGearsOnGround(double landingGearCompression_1, double landingGearCompression_2) {
  double numberOnGround = 0.0;
  if (landingGearCompression_1 > 0.0) {
    numberOnGround += 1.0;
  }
  if (landingGearCompression_2 > 0.0) {
    numberOnGround += 1.0;
  }
  return numberOnGround;
}

bool SpoilersHandler::areAtOrBelowIdle(double thrustLeverAngle_1, double thrustLeverAngle_2) {
  return thrustLeverAngle_1 <= TLA_IDLE && thrustLeverAngle_2 <= TLA_IDLE;
}

bool SpoilersHandler::areBelowClimb(double thrustLeverAngle_1, double thrustLeverAngle_2) {
  return thrustLeverAngle_1 < TLA_CLB && thrustLeverAngle_2 < TLA_CLB;
}

bool SpoilersHandler::isAtLeastOneInReverseAndOtherInIdle(double thrustLeverAngle_1, double thrustLeverAngle_2) {
  return (thrustLeverAngle_1 < TLA_IDLE && thrustLeverAngle_2 == TLA_IDLE) ||
         (thrustLeverAngle_2 < TLA_IDLE && thrustLeverAngle_1 == TLA_IDLE);
}

bool SpoilersHandler::isAtLeastOneInReverseAndOtherBelowMct(double thrustLeverAngle_1, double thrustLeverAngle_2) {
  return (thrustLeverAngle_1 < TLA_IDLE && thrustLeverAngle_2 < TLA_MCT) || (thrustLeverAngle_2 < TLA_IDLE && thrustLeverAngle_1 < TLA_MCT);
}
