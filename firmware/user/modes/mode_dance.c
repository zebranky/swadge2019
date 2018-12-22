/*
 * mode_dance.c
 *
 *  Created on: Nov 10, 2018
 *      Author: adam
 */

/*============================================================================
 * Includes
 *==========================================================================*/

#include "ccconfig.h"
#include "user_main.h"
#include <stdint.h>
#include "user_interface.h"
#include "user_main.h"
#include "osapi.h"
#include "embeddedout.h"
#include "custom_commands.h"

/*============================================================================
 * Defines
 *==========================================================================*/

#define SECRET_UNLOCK_MASK 0b11111111
#define SECRET_UNLOCK_CODE 0b10011001

/*============================================================================
 * Structs
 *==========================================================================*/

typedef struct
{
    os_timer_t timer;       ///< This is a system timer
    void (*timerFn)(void*); ///< This is a function which will be attached to the timer
    uint32_t period;        ///< This is the period, in ms, at which the function will be called
} timerWithPeriod;

/*============================================================================
 * Prototypes
 *==========================================================================*/

void ICACHE_FLASH_ATTR danceEnterMode(void);
void ICACHE_FLASH_ATTR danceExitMode(void);
void ICACHE_FLASH_ATTR danceButtonCallback(uint8_t state, int button, int down);
void ICACHE_FLASH_ATTR setDanceLeds(led_t* ledData, uint8_t ledDataLen);
uint32_t dance_rand(uint32_t upperBound);

void ICACHE_FLASH_ATTR unlockAnimation(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode1(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode2(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode3(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode4(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode5(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode6(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode7(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode8(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode9(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode10(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode11(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode12(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode13(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode14(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode15(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode16(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode17(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode18(void* arg);
void ICACHE_FLASH_ATTR freeze_color(void* arg);
void ICACHE_FLASH_ATTR random_dance_mode(void* arg);
/*============================================================================
 * Variables
 *==========================================================================*/

/**
 * This is the mode struct of callback pointers so the swadge knows how to
 * interact with this mode
 */
swadgeMode dancesMode =
{
    .modeName = "dances",
    .fnEnterMode = danceEnterMode,
    .fnExitMode = danceExitMode,
    .fnTimerCallback = NULL,
    .fnButtonCallback = danceButtonCallback,
    .fnAudioCallback = NULL,
    .wifiMode = NO_WIFI,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
};

static const uint8_t danceBrightnesses[] =
{
    0x01,
    0x08,
    0x40,
};
uint8_t danceBrightnessIdx = 0;

/**
 * This is an array of timerWithPeriod structs. Each is a separate animation
 * which is cycled around by button presses
 */
timerWithPeriod danceTimers[] =
{
    {
        .timer = {0},
        .timerFn = danceTimerMode1,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode2,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode3,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode4,
        .period = 5
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode5,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode6,
        .period = 80
    },


    {
        .timer = {0},
        .timerFn = danceTimerMode8,
        .period = 300
    },

    {
        .timer = {0},
        .timerFn = danceTimerMode9,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode10,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode11,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode12,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode13,
        .period = 2
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode14,
        .period = 2
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode15,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode16,
        .period = 120
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode17,
        .period = 100
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode18,
        .period = 15
    },
    {
        .timer = {0},
        .timerFn = danceTimerMode7,
        .period = 70
    },
    {
        .timer = {0},
        .timerFn = freeze_color,
        .period = 40
    },
    {
        .timer = {0},
        .timerFn = random_dance_mode,
        .period = 100
    }
};

/// Stuff for the secret unlock code
os_timer_t unlockAnimationTimer = {0};
uint8_t unlockBlinks = 0;
uint16_t buttonHistory = 0;

/// This is the current dance being animated
uint8_t currentDance = 0;

/// This is a state variable used in animations
int ledCount = 0;
int ledCount2 = 0;
int ledSwitch = 0;
int timerCount = 0;

int random_dance_timer = 0;
int random_choice = 8;

uint32_t color_save = 256;
uint32_t color_save_array[6] = {0};
uint32_t color_saturation_save[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint32_t current_color_array[6] = {0};

uint32_t current_color_saturation[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
int current_sparkle_save[6] =  { 0 };

bool led_bool = true;
/*============================================================================
 * Functions
 *==========================================================================*/

//min function
int min(int num1, int num2)
{
    return (num1 > num2 ) ? num2 : num1;
}


/**
 * This initializer is called whenever dance mode is entered
 */
void ICACHE_FLASH_ATTR danceEnterMode(void)
{
    // Loop through danceTimers[] and attach each function to each timer
    uint8_t i;
    for (i = 0; i < sizeof(danceTimers) / sizeof(danceTimers[0]); i++)
    {
        os_timer_disarm(&danceTimers[i].timer);
        os_timer_setfn(&danceTimers[i].timer, danceTimers[i].timerFn, NULL);
    }

    os_timer_disarm(&unlockAnimationTimer);
    os_timer_setfn(&unlockAnimationTimer, unlockAnimation, NULL);

    // Start the first timer in danceTimers[]
    os_timer_arm(&danceTimers[0].timer, danceTimers[0].period, true);
}

/**
 * This is called whenever dance mode is exited
 */
void ICACHE_FLASH_ATTR danceExitMode(void)
{
    // Loop through danceTimers[] and disarm timer
    uint8_t i;
    for (i = 0; i < sizeof(danceTimers) / sizeof(danceTimers[0]); i++)
    {
        os_timer_disarm(&danceTimers[i].timer);
    }
}

/**
 * This is called whenever a button is pressed or released
 * Button 1 will cycle through animations when pressed
 * Button 2 is currently unused
 *
 * @param state  A bitmask of all button states
 * @param button The button which caused this event
 * @param down   true if the button was pressed, false if it was released
 */
void ICACHE_FLASH_ATTR danceButtonCallback(uint8_t state __attribute__((unused)),
        int button, int down)
{
    // If a button was pressed, not released
    if(down)
    {
        // Check if the secret code was entered
        buttonHistory <<= 1;
        buttonHistory  |= (button == 1 ? 1 : 0);
        if(SECRET_UNLOCK_CODE == (SECRET_UNLOCK_MASK & buttonHistory))
        {
            // If it was, unlock all the patterns
            setGameWinsToMax();
            // Disarm the current animation, it will get rearmed after the pattern
            os_timer_disarm(&danceTimers[currentDance].timer);
            // Show a little thing
            unlockBlinks = 0;
            os_timer_arm(&unlockAnimationTimer, 200, true);
            return;
        }

        // Button 1 pressed
        if(2 == button)
        {
            // Stop the current animation
            os_timer_disarm(&danceTimers[currentDance].timer);

            // Iterate to the next animation. Winning a reflector game unlocks
            // another dance
            uint8_t numUnlockedDances = getRefGameWins();

            // If numUnlockedDances is 252 or less
            if(numUnlockedDances < 253)
            {
                // Add 3 to start with 3 dances unlocked, but don't overflow!
                numUnlockedDances += 3;
            }
            uint8_t numDances = sizeof(danceTimers) / sizeof(danceTimers[0]);
            if(numUnlockedDances > numDances)
            {
                numUnlockedDances = numDances;
            }

            currentDance = (currentDance + 1) % numUnlockedDances;

            // Start the next animation
            os_timer_arm(&danceTimers[currentDance].timer, danceTimers[currentDance].period, true);
        }
        else if(1 == button)
        {
            // Cycle brightnesses
            danceBrightnessIdx = (danceBrightnessIdx + 1) %
                                 (sizeof(danceBrightnesses) / sizeof(danceBrightnesses[0]));
        }
    }
}

/**
 * This animation is shown when the unlock code is entered
 * @param arg
 */
void ICACHE_FLASH_ATTR unlockAnimation(void* arg __attribute__((unused)))
{
    // Set the LEDs to either on (white) or off
    led_t leds[6] = {{0}};
    if(unlockBlinks % 2 == 0)
    {
        ets_memset(leds, 0x80, sizeof(leds));
    }
    setDanceLeds(leds, sizeof(leds));
    unlockBlinks++;

    // All done
    if(unlockBlinks == 8)
    {
        // Resume normal animation
        os_timer_disarm(&unlockAnimationTimer);
        os_timer_arm(&danceTimers[currentDance].timer, danceTimers[currentDance].period, true);
    }
}

/**
 * Intermediate function which adjusts brightness and sets the LEDs
 *
 * @param ledData    The LEDs to be scaled, then set
 * @param ledDataLen The length of the LEDs to set
 */
void ICACHE_FLASH_ATTR setDanceLeds(led_t* ledData, uint8_t ledDataLen)
{
    uint8_t i;
    for(i = 0; i < ledDataLen / sizeof(led_t); i++)
    {
        ledData[i].r = ledData[i].r / danceBrightnesses[danceBrightnessIdx];
        ledData[i].g = ledData[i].g / danceBrightnesses[danceBrightnessIdx];
        ledData[i].b = ledData[i].b / danceBrightnesses[danceBrightnessIdx];
    }
    setLeds(ledData, ledDataLen);
}

/**
 * Get a random number from a range.
 *
 * TODO
 * This isn't true-random, unless bound is a power of 2. But it's close enough?
 * The problem is that os_random() returns a number between [0, 2^64), and the
 * size of the range may not be even divisible by bound
 *
 * For what it's worth, this is what Arduino's random() does. It lies!
 *
 * @param bound An upper bound of the random range to return
 * @return A number in the range [0,bound), which does not include bound
 */
uint32_t dance_rand(uint32_t bound)
{
    return os_random() % bound;
}

/**
 * This animation is set to be called every 1 ms
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode1(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > 5)
    {
        ledCount = 0;
    }

    // Turn the current LED on, full bright white
    leds[ledCount].r = 255;
    leds[ledCount].g = 255;
    leds[ledCount].b = 255;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * This animation is set to be called every 100ms
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode2(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount < 5)
    {

        // Turn the current LED on, full bright red
        leds[0].r = 200;
        leds[1].r = 200;
        leds[2].r = 200;
        leds[3].r = 200;
        leds[4].r = 200;
        leds[5].r = 200;
        // Output the LED data, actually turning them on
    }
    else
    {
        // Turn the current LED on, full bright red
        leds[0].r = 0;
        leds[1].r = 0;
        leds[2].r = 0;
        leds[3].r = 0;
        leds[4].r = 0;
        leds[5].r = 0;
        // Output the LED data, actually turning them on

    }

    if(ledCount > 10)
    {
        ledCount = 0;
    }


    setDanceLeds(leds, sizeof(leds));
}



/**
 * This animation is set to be called every 100ms
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode3(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount - 1;
    if(ledCount < 0 || ledCount > 5)
    {
        ledCount = 5;
    }

    // Turn the current LED on, full bright red
    leds[ledCount].r = 0;
    leds[ledCount].g = 0;
    leds[ledCount].b = 255;

    leds[(6 - ledCount ) % 6].r = 0;
    leds[(6 - ledCount ) % 6].g = 0;
    leds[(6 - ledCount ) % 6].b = 255;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}


/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode4(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > 256)
    {
        ledCount = 0;
    }

    uint8_t i;
    for(i = 0; i < 6; i++)
    {
        int16_t angle = (((i * 256)  / 6)) + ledCount % 256;
        uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

        leds[i].r = (color >>  0) & 0xFF;
        leds[i].g = (color >>  8) & 0xFF;
        leds[i].b = (color >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}



/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode5(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > 5)
    {
        ledCount = 0;
    }

    uint8_t i;
    for(i = 0; i < 6; i++)
    {
        int16_t angle = (((i * 256)  / 6)) % 256;
        uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

        leds[(i + ledCount) % 6].r = (color >>  0) & 0xFF;
        leds[(i + ledCount) % 6].g = (color >>  8) & 0xFF;
        leds[(i + ledCount) % 6].b = (color >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}


/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode6(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount - 1;
    if(ledCount < 0 || ledCount > 255)
    {
        ledCount = 255;
    }
    int16_t angle = ledCount % 256;
    uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

    leds[(ledCount) % 6].r = (color >>  0) & 0xFF;
    leds[(ledCount) % 6].g = (color >>  8) & 0xFF;
    leds[(ledCount) % 6].b = (color >> 16) & 0xFF;



    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}


/**
 * slowly changes colors
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode7(void* arg __attribute__((unused)))
{
    os_printf("777\n" );
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > 255)
    {
        ledCount = 0;
    }
    int16_t angle = ledCount % 256;
    color_save = EHSVtoHEX(angle, 0xFF, 0xFF);

    uint8_t i;
    for(i = 0; i < 6; i++)
    {

        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}



/**
 * counts up in bianry
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode8(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;
    ledCount2 = ledCount2 + 1;
    if(ledCount2 > 75)
    {
        led_bool = !led_bool;
        ledCount2 = 0;
    }
    if(ledCount > 255)
    {
        ledCount = 0;
    }
    int16_t angle = ledCount % 256;
    uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

    uint8_t i;
    uint8_t j;
    for(i = 0; i < 6; i++)
    {
        if(ledCount2 >= 64)
        {
            leds[i].r = (color >>  0) & 0xFF;
            leds[i].g = (color >>  8) & 0xFF;
            leds[i].b = (color >> 16) & 0xFF;
        }
        else
        {

            if(led_bool)
            {
                j = 6 - i;
            }
            else
            {
                j = i;
            }
            if((ledCount2 >> i) & 1)
            {
                leds[(j) % 6].r = (color >>  0) & 0xFF;
                leds[(j) % 6].g = (color >>  8) & 0xFF;
                leds[(j) % 6].b = (color >> 16) & 0xFF;
            }
            else
            {
                leds[(j) % 6].r = 70;
                leds[(j) % 6].g = 0;
                leds[(j) % 6].b = 200;
            }
        }
    }
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * fire pattern
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode9(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;

    if(ledCount > 255)
    {
        ledCount = 0;
    }

    leds[3].r = dance_rand(120) + 135;
    leds[3].g = leds[3].r / 5;
    leds[4].r = dance_rand(80) + 80;
    leds[4].g = leds[4].r / 5;
    leds[5].r = dance_rand(50) + 40;
    leds[5].g = leds[5].r / 5;
    leds[0].r = dance_rand(10) + 10;
    leds[0].g = leds[0].r / 5;
    leds[2].r = dance_rand(80) + 80;
    leds[2].g = leds[2].r / 5;
    leds[1].r = dance_rand(50) + 40;
    leds[1].g = leds[1].r / 5;
    setDanceLeds(leds, sizeof(leds));
}


/**
 * fire pattern
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode10(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;

    if(ledCount > 255)
    {
        ledCount = 0;
    }

    leds[3].g = dance_rand(120) + 135;
    leds[3].b = leds[3].g / 5;
    leds[4].g = dance_rand(80) + 80;
    leds[4].b = leds[4].g / 5;
    leds[5].g = dance_rand(50) + 40;
    leds[5].b = leds[5].g / 5;
    leds[0].g = dance_rand(10) + 10;
    leds[0].b = leds[0].g / 5;
    leds[2].g = dance_rand(80) + 80;
    leds[2].b = leds[2].g / 5;
    leds[1].g = dance_rand(50) + 40;
    leds[1].b = leds[1].g / 5;
    setDanceLeds(leds, sizeof(leds));
}



/**
 * fire pattern
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode11(void* arg __attribute__((unused)))
{
    led_t leds[6] = {{0}};

    ledCount = ledCount + 1;

    if(ledCount > 255)
    {
        ledCount = 0;
    }

    leds[3].b = dance_rand(120) + 135;
    leds[3].g = leds[3].b / 5;
    leds[4].b = dance_rand(80) + 80;
    leds[4].g = leds[4].b / 5;
    leds[5].b = dance_rand(50) + 40;
    leds[5].g = leds[5].b / 5;
    leds[0].b = dance_rand(10) + 10;
    leds[0].g = leds[0].b / 5;
    leds[2].b = dance_rand(80) + 80;
    leds[2].g = leds[2].b / 5;
    leds[1].b = dance_rand(50) + 40;
    leds[1].g = leds[1].b / 5;
    setDanceLeds(leds, sizeof(leds));
}

/**
 * This animation is set to be called every 1 ms
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode12(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > 5)
    {
        ledCount = 0;
        color_save = EHSVtoHEX(dance_rand(256), 0xFF, 0xFF);
    }

    // Turn the current LED on, full bright white
    leds[ledCount].r = (color_save >>  0) & 0xFF;
    leds[ledCount].g = (color_save >>  8) & 0xFF;
    leds[ledCount].b = (color_save >> 16) & 0xFF;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode13(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;


    if(ledCount > 510)
    {
        ledCount = 0;
        ledCount2 = dance_rand(256);
    }
    int intensity = ledCount;
    if(ledCount > 255)
    {
        intensity = 510 - ledCount;
    }
    color_save = EHSVtoHEX(ledCount2, 0xFF, intensity);
    uint8_t i;
    for(i = 0; i < 6; i++)
    {

        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}
void ICACHE_FLASH_ATTR danceTimerMode14(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 2;
    if(ledCount > 5)
    {
        ledCount = 0;
    }

    // Turn the current LED on, full bright red
    leds[2].r = 255;
    leds[2].g = 140;
    leds[2].b = 10;

    leds[4].r = 255;
    leds[4].g = 140;
    leds[4].b = 10;

    leds[0].r = 255;
    leds[0].g = 140;
    leds[0].b = 10;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

void ICACHE_FLASH_ATTR danceTimerMode15(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    timerCount++;
    if (timerCount > 600)
    {
        timerCount = 0;
    }

    if (timerCount < 300)
    {
        ledSwitch = 0;
    }

    if (timerCount > 300)
    {
        ledSwitch = 1;
    }

    if(ledSwitch == 0)
    {
        // Turn the current LED on GREEN
        leds[0].r = 13;
        leds[0].g = 255;
        leds[0].b = 32;

        leds[1].r = 40;
        leds[1].g = 80;
        leds[1].b = 50;

        leds[2].r = 13;
        leds[2].g = 255;
        leds[2].b = 32;

        leds[3].r = 152;
        leds[3].g = 113;
        leds[3].b = 20;

        leds[4].r = 13;
        leds[4].g = 255;
        leds[4].b = 32;

        leds[5].r = 40;
        leds[5].g = 80;
        leds[5].b = 50;
    }

    if(ledSwitch == 1)
    {
        // Turn the current LED on RED
        leds[0].r = 255;
        leds[0].g = 32;
        leds[0].b = 32;

        leds[1].r = 80;
        leds[1].g = 50;
        leds[1].b = 50;

        leds[2].r = 255;
        leds[2].g = 32;
        leds[2].b = 32;

        leds[3].r = 152;
        leds[3].g = 113;
        leds[3].b = 20;

        leds[4].r = 255;
        leds[4].g = 32;
        leds[4].b = 32;

        leds[5].r = 80;
        leds[5].g = 50;
        leds[5].b = 50;


    }



    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));

}



/**
 * police sirens
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode16(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > 5)
    {
        ledCount = 0;

    }
    //green is 0
    //100 is fusia
    //70 is orange ish
    uint32_t red = EHSVtoHEX(85, 0xFF, 0xFF);
    uint32_t blue = EHSVtoHEX(170, 0xFF, 0xFF);
    uint8_t i;
    // Turn the current LED on, full bright white
    if(ledCount < 3)
    {
        for(i = 0; i < 3; i++)
        {
            leds[i].r = (red >>  0) & 0xFF;
            leds[i].g = (red >>  8) & 0xFF;
            leds[i].b = (red >> 16) & 0xFF;
        }
    }
    else
    {
        for(i = 3; i < 6; i++)
        {
            leds[i].r = (blue >>  0) & 0xFF;
            leds[i].g = (blue >>  8) & 0xFF;
            leds[i].b = (blue >> 16) & 0xFF;
        }
    }


    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}




/**
 * random blink
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode17(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[6] = {{0}};
    uint32_t rand_color = EHSVtoHEX(dance_rand(255), 0xFF, 0xFF);
    int rand_light = dance_rand(6);
    leds[rand_light].r = (rand_color >>  0) & 0xFF;
    leds[rand_light].g = (rand_color >>  8) & 0xFF;
    leds[rand_light].b = (rand_color >> 16) & 0xFF;
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}


/**
 * christmas lights
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode18(void* arg __attribute__((unused)))
{

    // Declare some LEDs, all off
    led_t leds[6] = {{0}};
    ledCount += 1;
    if(ledCount > ledCount2)
    {
        os_printf("greater\n" );
        ledCount = 0;
        ledCount2 = dance_rand(500) + 50;
        int color_picker = dance_rand(4);
        int node_select = dance_rand(6);

        if(color_picker < 2)
        {
            color_save_array[node_select] = 0;
            color_saturation_save[node_select] = dance_rand(15) + 240;
            current_sparkle_save[node_select] = dance_rand(20);
        }
        else if (color_picker < 4)
        {
            color_save_array[node_select] = 86;
            color_saturation_save[node_select] = dance_rand(15) + 240;
            current_sparkle_save[node_select] = dance_rand(20);
        }
        else
        {
            color_saturation_save[node_select] = dance_rand(25);
        }
    }


    uint8_t i;
    for(i = 0; i < 6; i++)
    {
        if(current_color_array[i] > color_save_array[i])
        {
            current_color_array[i] -= 1;
        }
        else if (current_color_array[i] < color_save_array[i])
        {
            current_color_array[i] += 1;
        }

        if(current_color_saturation[i] > color_saturation_save[i])
        {
            current_color_saturation[i] -= 1;
        }
        else if (current_color_saturation[i] < color_saturation_save[i])
        {
            current_color_saturation[i] += 1;
        }
    }

    for(i = 0; i < 6; i++)
    {
        leds[i].r = (EHSVtoHEX(current_color_array[i],  current_color_saturation[i], dance_rand(55) + 200) >>  0) & 0xFF;
        leds[i].g = (EHSVtoHEX(current_color_array[i],  current_color_saturation[i], dance_rand(55) + 200) >>  8) & 0xFF;
        leds[i].b = (EHSVtoHEX(current_color_array[i],  current_color_saturation[i], dance_rand(55) + 200) >> 16) & 0xFF;
    }
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}




/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR freeze_color(void* arg __attribute__((unused)))
{


    led_t leds[6] = {{0}};
    uint8_t i;
    for(i = 0; i < 6; i++)
    {
        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }
    setDanceLeds(leds, sizeof(leds));

}

/**
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR random_dance_mode(void* arg __attribute__((unused)))
{
    // os_printf("repeating\n" );
    random_dance_timer += 1;
    if(random_dance_timer > 20)
    {
        random_dance_timer = 0;
        uint8_t numDances = sizeof(danceTimers) / sizeof(danceTimers[0]);
        random_choice = dance_rand(numDances - 1);
    }
    danceTimers[random_choice].timerFn(arg);

}
