#include "buzzer.h"


static const BuzzerNote attack_notes[] = {
    { NOTE_E5, 60, 60 },
    { NOTE_G5, 70, 60 },
    { NOTE_A5, 80, 80 },
    { NOTE_E5, 60, 60 },
    { NOTE_G5, 70, 60 },
    { NOTE_A5, 80, 80 }   
};

static const BuzzerNote hit_notes[] = {
    { NOTE_C5, 80, 100 },
    { NOTE_G4, 60, 120 },
};

static const BuzzerNote startup_notes[] = {
    { NOTE_C4, 40, 80 },
    { NOTE_E4, 50, 80 },
    { NOTE_G4, 60, 80 },
    { NOTE_C5, 70, 120 },
};

static const BuzzerSequence buzzer_table[] = {
    { attack_notes,  sizeof(attack_notes)  / sizeof(BuzzerNote) },
    { hit_notes,     sizeof(hit_notes)     / sizeof(BuzzerNote) },
    { startup_notes, sizeof(startup_notes) / sizeof(BuzzerNote) },
};
void Audio_Play(BuzzerSound sound)
{
    if (sound >= BUZZER_SOUND_COUNT) return;
        Buzzer_Play(&buzzer_table[sound]);
}

bool Audio_isPlaying(BuzzerSound sound)
{
    return Buzzer_isPlaying();
}

bool Audio_Stop(BuzzerSound sound)
{
    Buzzer_Stop();
}