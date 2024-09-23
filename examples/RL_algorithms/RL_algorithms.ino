/**************************************
/ Arduino sketch using the Posit library to demonstrate Reinforcement Learning algorithms 
/ on an Arduino board.
/ 
/ In scope are : 
/ - Contextual bandits
/ - SARSA
/ - DQN
/ - Actor Critic
/
/ The target is to evaluate the feasibility of these algorithms on a simple Arduino Uno, despite its limitations
/ in terms of code size (30kB) and RAM (2kB). It is expected that only simple usages could fit, but that would
/ demonstrate the efficiency of Posits !
/ 
/ The target application is a (simulated) intelligent heating system, that could adapt the power to some parameters. 
/ ***************************************/

#define ES8 2 // Need for integer temps
#include "Posit.h"
// First step : conceptual bandits.

Posit8 wantedTemp=internalTemp=22 ; // The "context" variables
Posit8 externalTemp=15 ;
Posit16 generatedHeat=0 ; The result of heating
Posit16 energyUsed=0 ; the consumption of the system
uint8_t action=0 ; the number of the seleccted action 
Posit16

void actionNop () { // do nothing
  generatedHeat = energyUsed = 0;
  internalTemp = (internalTemp * 7 + externalTemp) /8; // averaging towards external temperature
}
void actionHeat () {
  generatedHeat = Posit16(21000) //
  energyUsed = Posit16(7000); // Heatpump
  internalTemp ++; // effect of heating
}



Posit16 calculateReward() {
  if (internalTemp < wantedTemp) return generatedHeat-energyUsed ;
  return -energyUsed; // If temperature high enough, no reward for result
}

setup() {
  // Create environment
  
} // end of setup()

loop() {
  // Execute iterations
  action = selectAction();
  executeAction(action);
  reward = calculateReward();
  updateValues(reward);
} // end of loop()

 
