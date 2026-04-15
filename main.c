#include "neuron.h"
#include <GL/glu.h>
#include <stdio.h>
#include <math.h>
#include "GL/glut.h"


Neuron neurons[MAX_NEURONS];
Synapse synapses[MAX_SYNAPSES];
int n_neurons = 0;
int n_synapses = 0;
double sim_time = 0.0;
float angleX = 0.0f;
float angleY = 0.0f;
int frame_delay = 10; // milliseconds between each calculation (default 10ms)
float zoom = 600.0f; // initial zoom level for the 3D view
double stim_remaining_ms = 0.0;
double stim_amp_nA = 120.0;


#define HISTORY_SIZE 200
float stim_history[MAX_NEURONS][HISTORY_SIZE];
int history_index = 0;

void update_plots_data(void);


// function to initialize the history buffer for plotting
void init_plots() {
    for (int i = 0; i < MAX_NEURONS; i++) {
        for (int j = 0; j < HISTORY_SIZE; j++) {
            stim_history[i][j] = 0.0f;
        }
    }
}

// initialize a neuron to be ready for stimulation (used when pressing spacebar)
static void rearm_neuron_for_stim(Neuron* n) {
    for (int i = 0; i < n->n_comparts; i++) {
        n->V[i] = -65.0;
        n->m[i] = 0.053;
        n->h[i] = 0.596;
        n->n[i] = 0.318;
        n->I_ext[i] = 0.0;
        n->vis_trace[i] = 0.0;
    }
}

void simulate_all() {
    double dt = 0.1;

    neurons[0].I_ext[neurons[0].soma_id] = stim_amp_nA;
    if (stim_remaining_ms > 0) {
        int s = neurons[0].soma_id;
        neurons[0].I_ext[s] = stim_amp_nA; 
        stim_remaining_ms -= dt;
    } else if (n_neurons > 0) {
        int s = neurons[0].soma_id;
        neurons[0].I_ext[s] = 0.0;
    }

    for (int i = 0; i < n_neurons; i++) {
        simulate_hh(&neurons[i], dt);
    }
    
    update_stdp(dt);
    sim_time += dt;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // camera position
    gluLookAt(0, 0, zoom, 0, 0, 0, 0, 1, 0);
    glRotatef(angleX, 1, 0, 0);
    glRotatef(angleY, 0, 1, 0);

    // draw neurons
    for (int i = 0; i < n_neurons; i++) {
        render_neuron(&neurons[i]);
    }

    // draw synapses
    for (int i = 0; i < n_synapses; i++) {
        render_synapse(&synapses[i]);
    }

    draw_ui_plots();
    glutSwapBuffers();
}

void timer(int v) {
    for (int i = 0; i < 5; i++) {
        simulate_all();
        update_plots_data();
    }

    glutPostRedisplay();
    glutTimerFunc(frame_delay, timer, 0);
}



// function to update the history buffer for plotting
void update_plots_data(void) {
    for (int i = 0; i < n_neurons; i++) {
        int s = neurons[i].soma_id;
        stim_history[i][history_index] = (float)neurons[i].I_ext[s];
    }
    history_index = (history_index + 1) % HISTORY_SIZE;
}
// moove camera with arrow keys
void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_UP)    angleX += 5.0f;
    if (key == GLUT_KEY_DOWN)  angleX -= 5.0f;
    if (key == GLUT_KEY_LEFT)  angleY -= 5.0f;
    if (key == GLUT_KEY_RIGHT) angleY += 5.0f;

    glutPostRedisplay();
}
void keyboard(unsigned char k, int x, int y) {
    
    //zoom
    if (k == '+' || k == '=') zoom -= 20.0f;
    if (k == '-' || k == '_') zoom += 20.0f;

    //stimulus
    if (k == ' ') {

        if (n_neurons > 0) {
   for (int i = 0; i < n_neurons; i++) {
        rearm_neuron_for_stim(&neurons[i]);
    }

    stim_remaining_ms = 2.5; // Duration of the stimulus in milliseconds
    printf("Network reset and stimulus started.\n");
}
    }
    //add neuron from SWC file 
    if (k == 'n') {
        if (n_neurons < MAX_NEURONS) {
            load_swc("data/simple.swc", &neurons[n_neurons]);
            neurons[n_neurons].id = n_neurons;
            // space out new neuron
            for(int i=0; i<neurons[n_neurons].n_comparts; i++) neurons[n_neurons].x[i] += 150.0 * n_neurons;
            n_neurons++;
            printf(" %d neuron added to the scene.\n", n_neurons-1);
        }
    }
    if (k == 'c') {
        //add synaps between last two neurons
       if (n_neurons >= 2) {
        add_synapse(n_neurons - 2, n_neurons - 1);
    } else {
        printf("Error: At least 2 neurons required for a synapse!\n");
    }
    }
    
    if (k == 's') { 
        frame_delay += 5; // Delay of -5ms to slow down the animation
        printf("Delay: %d ms\n", frame_delay);
    }
    if (k == 'f' && frame_delay > 1) { 
        frame_delay -= 5; // Delay of +5ms to speed up the animation
        printf("Velocity: %d ms\n", frame_delay);
    }
    
    if (k == 'h') {
        stim_amp_nA += 10.0; // Increases the stimulus intensity by 10 nA
        printf("Stimulus intensity increased to: %.1f nA\n", stim_amp_nA);
    }
    if (k == 'l') {
        stim_amp_nA -= 10.0; // Decreases the stimulus intensity by 10 nA
        if (stim_amp_nA < 0) stim_amp_nA = 0;
        printf("Stimulus intensity decreased to: %.1f nA\n", stim_amp_nA);
    }
    if (k == 'r') {
    //reser time 
    sim_time = 0.0;
    stim_remaining_ms = 0.0;

    //reset biophysical variables
    for (int i = 0; i < n_neurons; i++) {
        rearm_neuron_for_stim(&neurons[i]);
        for (int j = 0; j < HISTORY_SIZE; j++) stim_history[i][j] = 0.0f;
    }

    // reset weight
    for (int i = 0; i < n_synapses; i++) {
        synapses[i].weight = 1.0; 
    }

    printf("Biophysical variables and synaptic weights reset.\n");
}

    glutPostRedisplay();
}

void reshape(int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)width / (float)height, 1.0, 2500.0);
    glMatrixMode(GL_MODELVIEW);
}


void init_opengl() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f); 

    //lights
    GLfloat light_pos[] = { 200.0, 200.0, 200.0, 1.0 };
    GLfloat white_light[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    
    //projection setup
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 1.0, 2500.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("C-Neuron3D-Simulator");

    init_opengl();

    // load initial neuron from SWC file (default: data/simple.swc)
    load_swc("data/simple.swc", &neurons[0]);
    neurons[0].id = 0;
    n_neurons = 1;

    //callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys); 
    
    glutTimerFunc(10, timer, 0);
    glutKeyboardFunc(keyboard);

    printf("Simulator Started.\n");
    printf("COMMANDS:\n [SPACE] - Electrical stimulus\n [A] - Charge neuron\n [C] - Connect synapses\n");
    
    init_plots(); //initialize history buffer for plotting

    glutMainLoop();
    return 0;
}

