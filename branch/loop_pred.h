#include <inttypes.h>

#define INDEX 6    // Number of bits required to index into the table
#define TAG_BITS 14  // Number of bits to represent tag in the table
#define AGE 31      // Intiial timer of the the entry


// class table_entry
// {
// public:
//     uint8_t counter;
//     uint16_t tag;
//     uint8_t useful;
// };

// class Bimodal
// {
// public:
//     uint8_t bimodal_table[BIMODAL_TABLE_SIZE];
// };

// class Tage
// {
// public:
//     int num_branches; // Stores the number of branch instructions since the last useful reset
//     Bimodal Base_Predictor; // Array represent the counters of the bimodal table
//     table_entry prediction_table[NUM_OF_COMPONENTS][(1 << MAX_INDEX_BITS)];
//     uint8_t global_history[GLOBAL_HISTORY_BUFFER]; // Stores the global branch history
//     uint8_t path_history[PATH_HISTORY_BUFFER]; // Stores the last bits of the last N branch PCs
//     uint8_t use_alt; // 4 bit counter to decide between alternate and provider component prediction
//     int component_history_lengths[NUM_OF_COMPONENTS]; // History lengths used to compute hashes for different components
//     uint8_t prediction, prediction_tage, prediction_alternate; // Final prediction , provider prediction, and alternate prediction
//     int provider_component, alternate_component; // Provider and alternate component of last branch PC
//     int strength; //Strength of provider prediction counter of last branch PC


class Entry
{
public:
    uint16_t tag;          
    uint16_t past;
    uint16_t current;
    uint8_t timer;
    uint8_t confidence;
};

const int entries = 256;

class Loop_Predictor
{
public:
    Entry table[entries]; // Predictor table
    
    int way = 4;
    int max_iter = 14; 
    uint8_t hash;

    int hit;              
    int ptag;             
    int ind;              
    int is_valid;    
    uint8_t prediction_loop; 

    void free_entry(int index)
    {
        table[index].past = 0;
        table[index].timer = 0;
        table[index].confidence = 0;
    }

    void set(int index, int tag)
    {
        free_entry(index);
        table[index].tag = tag;
        table[index].current = 1;
        table[index].timer = AGE;
    }

    void init()
    {
        hash = 0;
        for (int i = 0; i < entries; i++)
        {
            set(i, 0);
            free_entry(i);
            table[i].current = 0;
        }
    }
    
    void change_time(int index, bool increment)
    {
        if(increment) table[index].timer++;
        else table[index].timer--;
    }

    uint8_t get_prediction(uint64_t pc)
    {
        ind = (pc & ((1 << INDEX) - 1)) << 2; 
        int mask = ((1 << TAG_BITS) - 1);
        ptag = (pc >> INDEX) & mask;    
        
        prediction_loop = 0;
        is_valid = 0;

        int i = ind;

        while(i < ind + way)
        {
            if (table[i].tag == ptag)
            {
                hit = i;
                if(table[i].confidence == 3) is_valid = 1;

                if (table[i].current - table[i].past == -1)
                {
                    return 0;
                }
                prediction_loop = 1;
                return 1;
            }
            i++;
        }
        hit = -1;
        return 0;
    }

    void update(uint8_t taken, uint8_t tage_pred)
    {
        if (hit >= 0)
    {
        Entry &entry = table[ind + hit];
        if (is_valid)
        {
            // If the predicton was wrong, free the entry
            if (taken != prediction_loop)
            {
                free_entry(ind + hit);
                entry.current = 0;
                return;
            }

            if (taken != tage_pred)
            {
                if (entry.timer < AGE)
                    change_time(ind + hit, 1);
            }
        }

        entry.current++;
        entry.current &= ((1 << max_iter) - 1);

        // If the iteration is greater than what was seen last time, free the entry
        if (entry.current > entry.past)
        {
            entry.confidence = 0;
            if (entry.past != 0)
            {
                free_entry(ind + hit);
            }
        }

        if (!taken)
        {
            if (entry.current == entry.past)
            {
                // Increase the confidence if correct
                if (entry.confidence < 3)
                    entry.confidence++;
                
                // We do not care for loops with < 3 iterations
                if ((entry.past > 0) && (entry.past < 3))
                {
                    free_entry(ind + hit);
                }
            }
            else
            {
                // Set the newly allocated entry
                if (entry.past == 0)
                {
                    entry.confidence = 0;
                    entry.past = entry.current;
                }
                // else free the entry
                else
                {
                    free_entry(ind + hit);
                }
            }
            entry.current = 0;
        }
    }
    // If the branch is taken but there is no entry, we must allocate one entry in the table
    else if (taken)
    {
        hash = (hash + 1) % 4;
        for (int i = 0; i < 4; i++)
        {
            int j = ind + ((hash + i) % 4);
            if (table[j].timer == 0)
            {
                set(j, ptag);
                break;
            }
            else if (table[j].timer > 0)
                change_time(j, 0);
        }
    }
    }

};
