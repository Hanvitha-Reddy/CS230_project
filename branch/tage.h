#include "ooo_cpu.h"

#define BIMODAL_TABLE_SIZE 16384
#define MAX_INDEX_BITS 12
#define NUM_OF_COMPONENTS 12 // TODO
#define BASE_COUNTER 2
#define COUNTER 3
#define USEFUL 2
#define GLOBAL_HISTORY_BUFFER 1024
#define PATH_HISTORY_BUFFER 32
#define GEOMETRIC_RATIO 1.6
#define RESET_USEFUL_INTERVAL 512000

uint8_t Index_Length[NUM_OF_COMPONENTS] = {10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9};
uint8_t Tag_Length[NUM_OF_COMPONENTS] = {7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15};

class table_entry
{
public:
    uint8_t counter;
    uint16_t tag;
    uint8_t useful;
};

class Bimodal
{
public:
    uint8_t bimodal_table[BIMODAL_TABLE_SIZE];
};

class Tage
{
public:
    int num_branches; // Stores the number of branch instructions since the last useful reset
    Bimodal Base_Predictor; // Array represent the counters of the bimodal table
    table_entry prediction_table[NUM_OF_COMPONENTS][(1 << MAX_INDEX_BITS)];
    uint8_t global_history[GLOBAL_HISTORY_BUFFER]; // Stores the global branch history
    uint8_t path_history[PATH_HISTORY_BUFFER]; // Stores the last bits of the last N branch PCs
    uint8_t use_alt; // 4 bit counter to decide between alternate and provider component prediction
    int component_history_lengths[NUM_OF_COMPONENTS]; // History lengths used to compute hashes for different components
    uint8_t prediction, prediction_tage, prediction_alternate; // Final prediction , provider prediction, and alternate prediction
    int provider_component, alternate_component; // Provider and alternate component of last branch PC
    int strength; //Strength of provider prediction counter of last branch PC


    void init()
    {
        num_branches = 0;
        use_alt = 8;
        prediction_tage = 0;
        for (int i = 0; i < BIMODAL_TABLE_SIZE; i++)
        {
            Base_Predictor.bimodal_table[i] = (1 << (BASE_COUNTER - 1)); // weakly taken
        }
        for (int i = 0; i < NUM_OF_COMPONENTS; i++)
        {
            for (int j = 0; j < (1 << Index_Length[i]); j++)
            {
                prediction_table[i][j].counter = (1 << (COUNTER - 1)); // weakly taken
                prediction_table[i][j].useful = {0};                           // not useful
                prediction_table[i][j].tag = {0};
            }
        }

        float start_number = 1;
        for (int i = 0; i < NUM_OF_COMPONENTS; i++)
        {
            component_history_lengths[i] = int((4 * start_number * pow(GEOMETRIC_RATIO, i)) + 0.5); // set component history lengths
        }
    }

    uint8_t get_prediction(uint64_t ip, int component)
    {
        if(component == 0) // Check if component is the bimodal table
        {
            int index = bimodal_index(ip); // Get bimodal index
            int boolean = Base_Predictor.bimodal_table[index] >= (1 << (BASE_COUNTER - 1));
            return boolean;
        }
        else
        {
            int index = predictor_index(ip, component); // Get component-specific index
            int boolean = prediction_table[component - 1][index].counter >= (1 << (COUNTER - 1));
            return boolean;
        }
    }

    uint8_t predict(uint64_t ip)
    {
        provider_component = get_highest_match(ip, NUM_OF_COMPONENTS + 1); // Get the first predictor from the end which matches the PC
        alternate_component = get_highest_match(ip, provider_component); // Get the first predictor below the provider which matches the PC 

        //Store predictions for both components for use in the update step
        prediction = get_prediction(ip, provider_component); 
        prediction_alternate = get_prediction(ip, alternate_component);

        prediction_tage = prediction;

        if(provider_component != 0)
        {
            int index = predictor_index(ip, provider_component);
            int counter_value = prediction_table[provider_component - 1][index].counter;
            int threshold = (1 << COUNTER) - 1;
            strength = abs(2 * counter_value - threshold) > 1;
            if (use_alt >= 8 && !strength) // Use provider component only if use_alt < 8 or the provider counter is strength
                prediction_tage = prediction_alternate;
        }

        return prediction_tage;
    }

    void update(uint64_t ip, uint8_t taken)
    {
        if (provider_component > 0)  // the predictor component is not the bimodal table
        {
            table_entry *entry = &prediction_table[provider_component - 1][predictor_index(ip, provider_component)];
            uint8_t useful = entry->useful;

            if(!strength )  
            {
                if (prediction != prediction_alternate)
                {
                    if (prediction !=taken ) 
                    {
                        if (use_alt< 15) use_alt++;
                        if(use_alt < 8 && entry->useful > 0) entry->useful--;  // if prediction from altpred component was correct
                        
                    }    
                    else
                    {
                        if (use_alt >0) use_alt--;
                        if (entry->useful < ((1 << USEFUL) - 1)) entry->useful++;  // if prediction from preditor component was correct
                    }
                }
            }
            
            else 
            { 
                if (prediction!=prediction_alternate)
                {
                    if (prediction ==taken)
                    {
                        if (entry->useful < ((1 << USEFUL) - 1)) entry->useful++;  // if prediction from preditor component was correct
                    }
                    else
                    {
                        if(use_alt <8 && entry->useful >0) entry->useful--;
                    }
                }
            }

            if(alternate_component > 0)  // alternate component is not the bimodal table
            {
                table_entry *alt_entry = &prediction_table[alternate_component - 1][predictor_index(ip, alternate_component)];
                if(useful == 0)
                {
                    if(taken && alt_entry->counter < ((1 << COUNTER) - 1))
                    {
                        alt_entry->counter++;
                    }
                    else if(!taken && alt_entry->counter > 0)
                    {
                        alt_entry->counter--;
                    }
                }
            }
            else
            {
                uint16_t index = bimodal_index(ip);
                if (useful == 0)
                {
                    if(taken && Base_Predictor.bimodal_table[index] < ((1 << BASE_COUNTER) - 1))
                    {
                        Base_Predictor.bimodal_table[index]++;
                    }
                    else if(!taken && Base_Predictor.bimodal_table[index] > 0)
                    {
                        Base_Predictor.bimodal_table[index]--;
                    }
                }
            }


            if(taken && entry->counter < ((1 << COUNTER) - 1))
            {
                entry->counter++;
            }
            else if(!taken && entry->counter > 0)
            {
                entry->counter--;
            }
        }

        else
        {
            uint16_t index = bimodal_index(ip);
            
            if(taken && Base_Predictor.bimodal_table[index] < ((1 << BASE_COUNTER) - 1)){
                Base_Predictor.bimodal_table[index]++;}
            else if(!taken && Base_Predictor.bimodal_table[index] > 0){
                Base_Predictor.bimodal_table[index]--;}
        }

        // allocate tagged entries on misprediction
        if (prediction_tage != taken)
        {
            long rand = random();
            int mask = ((1 << (NUM_OF_COMPONENTS)) - 1);
            rand = rand & mask;
            int start_component = provider_component + 1;

            //compute the start-component for search
            if (!(rand % 2) && (rand % 4))  // 0.5 probability
            {
                start_component++;
            }
            else if(!(rand % 4))
            {
                start_component += 2;
            }

            //Allocate atleast one entry if no free entry
            int not_useful = 0;
            for (int i = provider_component + 1; i <= NUM_OF_COMPONENTS; i++)
            {
                int index = predictor_index(ip, i);
                if (prediction_table[i - 1][index].useful == 0)
                    not_useful = 1;
            }
            if (!not_useful && (start_component <= NUM_OF_COMPONENTS)){
                int index = predictor_index(ip, start_component);
                prediction_table[start_component - 1][index].useful = 0;
            }
            
            // search for entry to steal from the start-component till end
            for (int i = start_component; i <= NUM_OF_COMPONENTS; i++)
            {
                int start_counter = (1 << (COUNTER - 1));
                if (!prediction_table[i - 1][predictor_index(ip, i)].useful)
                {
                    int index = predictor_index(ip, i);
                    prediction_table[i-1][index].tag = get_tag(ip, i);
                    prediction_table[i-1][index].counter = start_counter;
                    break;
                }
            }
        }
        num_branches++;
        // update global & path history
        for (int i = GLOBAL_HISTORY_BUFFER - 1; i > 0; i--){
            global_history[i] = global_history[i - 1];
            if(i <= PATH_HISTORY_BUFFER)
            {
                path_history[i] = path_history[i - 1];
            }
        }
        global_history[0] = taken;
        path_history[0] = ip % 2;
        
        // graceful resetting of useful counter
        if (num_branches == RESET_USEFUL_INTERVAL)
        {
            num_branches = 0;
            
            for (int i = 0; i < NUM_OF_COMPONENTS; i++)
            {
                int last = (1 << Index_Length[i]);
                for (int j = 0; j < last; j++)
                    prediction_table[i][j].useful /= 2;
            }
        }
    }

    uint16_t bimodal_index(uint64_t ip)
    {
        return ip & (BIMODAL_TABLE_SIZE - 1);
    }

    uint64_t hash_path_history(int component)
    {
        uint64_t A = 0;
        
        int size = component_history_lengths[component - 1]; // Size of hash output
        if(size > 16) size = 16;

        for (int i = PATH_HISTORY_BUFFER; i > 0; i--)
        {
            A = (A << 1);
            A = A | path_history[i-1]; // Build the bit vector a using the path history array
        }

        A = A & ((1 << size) - 1);
        uint64_t A1;
        uint64_t A2;
        int M = Index_Length[component - 1];

        A1 = A & ((1 << M) - 1); // Get last M bits of A
        A2 = (A >> M) & ((1 << M) - 1); // Get second last M bits of A

        // Use the hashing from the CBP-4 L-Tage submission
        A2 = ((A2 << component) & ((1 << M) - 1)) + (A2 >> abs(M - component));
        A = A1 ^ A2;
        A = ((A << component) & ((1 << M) - 1)) + (A >> abs(M - component));
        
        return (A);
    }

    uint64_t hash_global_history(int input, int output)
    {
        uint64_t final = 0; // Stores final compressed history
        uint64_t temp = 0; // Temorarily stores some bits of history

        for (int i = 0; i < input; i++)
        {
            if (i % output == 0)
            {
                final ^= temp; // XOR current segment into the compressed history
                temp = 0;
            }
            temp *= 2;
            if(global_history[i])
            {
                temp += 1;
            }
        }
        return (final ^ temp);
    }

    uint16_t predictor_index(uint64_t ip, int component)
    {
        uint64_t path_history_hash = hash_path_history(component); // Hash of path history

        // Hash of global history
        int M = Index_Length[component - 1];
        uint64_t global_history_hash = hash_global_history(component_history_lengths[component - 1], M);

        uint16_t index = (global_history_hash ^ ip ^ (ip >> (abs(M - component) + 1)) ^ path_history_hash) & ((1 << M) - 1);
        
        return index;
    }

    uint16_t get_tag(uint64_t ip, int component)
    {
        int M = Tag_Length[component - 1];
        uint64_t global_history_hash = hash_global_history(component_history_lengths[component - 1], M);
        global_history_hash ^= hash_global_history(component_history_lengths[component - 1], M - 1);
        
        return (global_history_hash ^ ip) & ((1 << M) - 1);
    }

    int get_highest_match(uint64_t ip, int component)
    {
        int i = component - 1;
        while(i > 0)
        {
            table_entry *entry = &prediction_table[i - 1][predictor_index(ip, i)];
            if(entry->tag == get_tag(ip, i))
            {
                return i;
            }
            i--;
        }
        
        return 0; // Default to bimodal in case no match found
    }
};