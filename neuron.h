#ifndef NEURON_H
#define NEURON_H

#include <GL/glu.h> 

#define MAX_COMPARTS 2000 // maximum number of compartments per neuron (SWC standard allows up to 9999, but we set a practical limit)
#define MAX_NEURONS 10    // maximum number of neurons in the 3D scene
#define MAX_SYNAPSES 100  // maximum number of synaptic connections


typedef struct {
    int id;
    int n_comparts;
    
    //3d geometry
    double x[MAX_COMPARTS], y[MAX_COMPARTS], z[MAX_COMPARTS]; // coordinates in μm 
    double r[MAX_COMPARTS];      // Radius of the compartment
    int type[MAX_COMPARTS];      // 1=Soma, 2=Axon, 3=Dendrite 
    int parent[MAX_COMPARTS];    // Index of parent compartment 

    double V[MAX_COMPARTS];      // membrane potential (mV) 
    double m[MAX_COMPARTS], h[MAX_COMPARTS], n[MAX_COMPARTS]; 
    
    //Biophysical Parameters
    double Cm;                   // membrane capacitance (µF/cm^2)
    double gNa, gK, gL;          // maximum conductances (Sodium, Potassium, Leak)
    double ENa, EK, EL;          // reversal potentials [5]

    int soma_id;                 // Index of the root node
    double last_spike_t;         // Time of the last spike (for STDP) [6]
    GLUquadric* quadric;         // For 3D rendering of spheres/cylinders

    double I_ext[MAX_COMPARTS]; // current injection (nA)
    double vis_trace[MAX_COMPARTS]; // Visual trace 
} Neuron;


typedef struct {
    int pre_neuron_id;           // pre-synaptic neuron ID
    int post_neuron_id;          // post-synaptic neuron ID
    int pre_compart;             // Index of pre-synaptic compartment (axonal terminal)
    int post_compart;            // Index of post-synaptic compartment (contact point)

    double weight;               // Strength of the synapse (visual thickness) 
    double delay;        // Delay based on SWC distance
    double trace_pre;    //Traccia spike pre-sinaptico
    double trace_post;   // Traccia spike post-sinaptico
   
    double last_pre_t;           // Timestamp spike pre-sinaptico per STDP
    double last_post_t;          // Timestamp spike post-sinaptico per STDP
} Synapse;


extern Neuron neurons[MAX_NEURONS];
extern Synapse synapses[MAX_SYNAPSES];
extern int n_neurons;
extern int n_synapses;
extern double sim_time;

void load_swc(const char* filename, Neuron* n);
void simulate_hh(Neuron* n, double dt);
void update_stdp(double dt);
void add_synapse(int pre_id, int post_id);
void render_neuron(Neuron* n);
void render_synapse(Synapse* s);
void draw_ui_plots();
#endif 
