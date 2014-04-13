#include "FitnessFunctions.h"
#include "InputModels/InputVector.h"

#include "string.h"

double FitnessFunctions::MonteCarloEfficiency(Keyboard& keyboard, InputModel& model, WordList& words, unsigned int iterations) {

    unsigned int matched = 0, missed = 0;
    for(unsigned int iteration = 0; iteration < iterations; iteration++) {
        const char *word = words.RandomWord();
        InputVector sigma = model.RandomVector(word, keyboard);

        const char *best_word = 0;
        double best_probability = 0;
        for(unsigned int i = 0; i < words.Words(); i++) {
            const char *possible_word = words.Word(i);
            const double probability = model.MarginalProbability(sigma, possible_word, keyboard);
            if(probability > best_probability) {
                best_word = possible_word;
                best_probability = probability;
            }
        }

        if(strcmp(word, best_word) == 0) {
            matched++;
        }
        else {
            missed++;
        }
    }

    return double(matched)/double(matched+missed);
}
