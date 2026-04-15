#include "neuron.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glu.h>

// functions for Hodgkin-Huxley model
double alpha_m(double V) { return 0.1 * (V + 40.0) / (1.0 - exp(-(V + 40.0) / 10.0)); }
double beta_m(double V)  { return 4.0 * exp(-(V + 65.0) / 18.0); }
double alpha_h(double V) { return 0.07 * exp(-(V + 65.0) / 20.0); }
double beta_h(double V)  { return 1.0 / (1.0 + exp(-(V + 35.0) / 10.0)); }
double alpha_n(double V) { return 0.01 * (V + 55.0) / (1.0 - exp(-(V + 55.0) / 10.0)); }
double beta_n(double V)  { return 0.125 * exp(-(V + 65.0) / 80.0); }

// swc morphology parser and HH simulator
void load_swc(const char* filename, Neuron* n) {
    FILE* fp = fopen(filename, "r");
    n->quadric = gluNewQuadric();
    if (!fp) {
        printf("Error: file not found '%s'\n", filename);
        return;
    }

    char line[500]; 
    n->n_comparts = 0;

    // initial voltage -65 mV for all compartments 
for (int i = 0; i < n->n_comparts; i++) {
    n->V[i] = -65.0; 
}

    while(fgets(line, sizeof(line), fp)) {
        if(line[0] == '#' || strlen(line) < 5) continue;
        int id, type, parent;
        double x, y, z, r;
        
        if (sscanf(line, "%d %d %lf %lf %lf %lf %d", &id, &type, &x, &y, &z, &r, &parent) == 7) {
            n->x[n->n_comparts] = x;
            n->y[n->n_comparts] = y;
            n->z[n->n_comparts] = z;
            n->r[n->n_comparts] = r;
            n->type[n->n_comparts] = type;
            n->parent[n->n_comparts] = (parent == -1) ? -1 : parent - 1; 
            if(type == 1) n->soma_id = n->n_comparts;
            n->n_comparts++;
        }
    }
    fclose(fp);
    
    // parameters for Hodgkin-Huxley model (standard values for squid giant axon)
    n->Cm = 1.0;    // membrane capacitance in μF/cm²
    n->gNa = 120.0; // conductance Na
    n->gK = 36.0;   // conductance K
    n->gL = 0.3;    // conductance Leak
    n->ENa = 50.0;  // reversal potential Na (mV)
    n->EK = -77.0;  // reversal potential K (mV)
    n->EL = -54.4;  // reversal potential Leak (mV)
    
    
    for (int i = 0; i < n->n_comparts; i++) {
        n->V[i] = -65.0;  
    }
    for (int i = 0; i < n->n_comparts; i++) {
            double am = alpha_m(n->V[i]), bm = beta_m(n->V[i]);
            double ah = alpha_h(n->V[i]), bh = beta_h(n->V[i]);
            double an = alpha_n(n->V[i]), bn = beta_n(n->V[i]);
            n->m[i] = am / (am + bm);
            n->h[i] = ah / (ah + bh);
            n->n[i] = an / (an + bn);
        n->I_ext[i] = 0.0;
        n->vis_trace[i] = 0.0;
    }
    
    n->quadric = gluNewQuadric();
    
    printf("File SWC '%s' : %d compartments, soma_id=%d\n", filename, n->n_comparts, n->soma_id);
}
// simulate a single neuron for one time step dt using the Hodgkin-Huxley model
void simulate_hh(Neuron* n, double dt) {
    double Itot[MAX_COMPARTS];
    double next_V[MAX_COMPARTS];

    for (int i = 0; i < n->n_comparts; i++) {
        // local ionic currents
        double INa = n->gNa * pow(n->m[i], 3) * n->h[i] * (n->V[i] - n->ENa);
        double IK  = n->gK * pow(n->n[i], 4) * (n->V[i] - n->EK);
        double IL  = n->gL * (n->V[i] - n->EL);
        Itot[i] = -(INa + IK + IL);
        // add external current
        Itot[i] += n->I_ext[i];
    }

    for (int i = 0; i < n->n_comparts; i++) {
        int p = n->parent[i];
        if (p >= 0) {
            double dist = sqrt(pow(n->x[i] - n->x[p], 2) +
                               pow(n->y[i] - n->y[p], 2) +
                               pow(n->z[i] - n->z[p], 2));
            double Rax = 0.7 * (dist + 1e-6); 
            double axial_I = (n->V[p] - n->V[i]) / Rax;
            Itot[i] += axial_I;
            Itot[p] -= axial_I;
        }
    }

    for (int i = 0; i < n->n_comparts; i++) {
        next_V[i] = n->V[i] + (dt * Itot[i] / n->Cm);
        
        // gate updates using Hodgkin-Huxley equations
        n->m[i] += dt * (alpha_m(n->V[i]) * (1.0 - n->m[i]) - beta_m(n->V[i]) * n->m[i]);
        n->h[i] += dt * (alpha_h(n->V[i]) * (1.0 - n->h[i]) - beta_h(n->V[i]) * n->h[i]);
        n->n[i] += dt * (alpha_n(n->V[i]) * (1.0 - n->n[i]) - beta_n(n->V[i]) * n->n[i]);

        if (n->V[i] < 0 && next_V[i] >= 0 && i == n->soma_id) n->last_spike_t = sim_time;
        n->V[i] = next_V[i];

        {
            //decay visual trace and add new depolarization
            double depol = (n->V[i] + 65.0) / 15.0;
            if (depol < 0.0) depol = 0.0;
            if (depol > 1.0) depol = 1.0;

            double decay = exp(-dt / 25.0); // tau visivo ~25 ms
            n->vis_trace[i] *= decay;
            if (depol > n->vis_trace[i]) n->vis_trace[i] = depol;
        }
    }
}

