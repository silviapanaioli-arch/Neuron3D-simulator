#include "neuron.h"
#include <math.h>
#include <stdio.h>

#define MAX_TOTAL_IN_WEIGHT 5  
#define TAU_TRACE 0.5
#define CONDUCTION_VELOCITY 0.5  

extern Neuron neurons[MAX_NEURONS];
extern Synapse synapses[MAX_SYNAPSES];
extern int n_neurons;
extern int n_synapses;
extern double sim_time;

void update_stdp(double dt) {

    double sum_weights[MAX_NEURONS] = {0};

    for (int i = 0; i < n_synapses; i++) {
        Synapse* s = &synapses[i];
        Neuron* pre = &neurons[s->pre_neuron_id];
        Neuron* post = &neurons[s->post_neuron_id];

        s->trace_pre *= exp(-dt / TAU_TRACE);
        s->trace_post *= exp(-dt / TAU_TRACE);


        if (sim_time - post->last_spike_t < dt && sim_time > 0.1) {
            s->trace_post += 1.0;
        }

        //based on Hebbs rule 
       if (pre->V[s->pre_compart] > 0.0) {
            post->I_ext[s->post_compart] += s->weight * 45.0; 
        }

        if (sim_time - post->last_spike_t < dt) {
        // increase weight if post spike just happened 
            s->weight += 0.01 * s->trace_pre;
        }
        if (sim_time - pre->last_spike_t < dt) {
            
            s->weight -= 0.005 * s->trace_post;
        }
    if (neurons[s->pre_neuron_id].V[s->pre_compart] > 0.0) {
                // Transmit current to the postsynaptic neuron
                neurons[s->post_neuron_id].I_ext[s->post_compart] += 40.0;
            } else {
                neurons[s->post_neuron_id].I_ext[s->post_compart] = 0.0;
        }
        // limits for weight to keep it in a reasonable range for visualization and stability
        if (s->weight < 0.01) s->weight = 0.01;
        if (s->weight > 1.0)  s->weight = 1.0;

        // accumulate total incoming weight 
        sum_weights[s->post_neuron_id] += s->weight;

        //phisiological delay
        double time_since_pre_spike = sim_time - pre->last_spike_t;
        
        if (time_since_pre_spike >= s->delay && time_since_pre_spike < s->delay + dt) {
            post->I_ext[s->post_compart] = 90.0 * s->weight; 
            
        }
        else {
            post->I_ext[s->post_compart] *= 0.8;  //decay of synaptic effect
        }
    }

    for (int j = 0; j < n_synapses; j++) {
        int target = synapses[j].post_neuron_id;
        if (sum_weights[target] > MAX_TOTAL_IN_WEIGHT) {
            synapses[j].weight *= (MAX_TOTAL_IN_WEIGHT / sum_weights[target]);
        }
    }
}


void add_synapse(int pre_id, int post_id) {
    if (n_synapses < MAX_SYNAPSES) {
        Synapse* s = &synapses[n_synapses];
        s->pre_neuron_id = pre_id;
        s->post_neuron_id = post_id;
        s->weight = 1.0;
        int axonal_terminal = -1;
        for (int i = 0; i < neurons[pre_id].n_comparts; i++) {
            if (neurons[pre_id].type[i] == 2) { 
                axonal_terminal = i; 
            }
        }
        

        //if no axonal terminal found, use last compartment as default (should be axonal in well-formed SWC)
        s->pre_compart = (axonal_terminal != -1) ? axonal_terminal : neurons[pre_id].n_comparts - 1;
        s->post_compart = neurons[post_id].soma_id; // contact point is soma for simplicity

        double dx = neurons[pre_id].x[s->pre_compart] - neurons[post_id].x[s->post_compart];
        double dy = neurons[pre_id].y[s->pre_compart] - neurons[post_id].y[s->post_compart];
        double dz = neurons[pre_id].z[s->pre_compart] - neurons[post_id].z[s->post_compart];
        double distance = sqrt(dx*dx + dy*dy + dz*dz);

        // delay (ms) = distance (um) / velocity (um/ms)
        s->delay = distance / CONDUCTION_VELOCITY; //conduction_velocity is 0.5 
        
        //initialization of weight and traces
            s->weight = 1.0;     // initial weight 
            s->trace_pre = 0.0;  // traces
        s->trace_post = 0.0;
        
        n_synapses++; 
        
        printf("Synapse %d created: %d -> %d [Delay: %.2f ms, Weight: %.1f]\n", 
                n_synapses-1, pre_id, post_id, s->delay, s->weight);
    }
    }
