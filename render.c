#include "neuron.h"
#include "GL/glut.h"
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define HISTORY_SIZE 200
#define PLOT_LEFT 790.0f
#define PLOT_RIGHT 1000.0f
#define PLOT_BOTTOM 530.0f
#define PLOT_TOP 740.0f
#define PLOT_TIME_STEP_MS 0.1f
#define PLOT_Y_PADDING_FACTOR 0.1f
extern float stim_history[MAX_NEURONS][HISTORY_SIZE];
extern int history_index;

extern Neuron neurons[MAX_NEURONS];

static void draw_text(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* p = text; *p != '\0'; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);
    }
}

//solid cylinder between two points (x1,y1,z1) and (x2,y2,z2) with radius r 
void draw_cylinder(double x1, double y1, double z1, double x2, double y2, double z2, double r, GLUquadric* q) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    double dist = sqrt(dx * dx + dy * dy + dz * dz);
    
   //angle and axis for rotation
    double rot = acos(dz / (dist + 1e-9)) * 180.0 / M_PI;
    double ax = -dy * dz;
    double ay = dx * dz;

    glPushMatrix();
        glTranslated(x1, y1, z1);
        glRotated(rot, ax, ay, 0);
        gluCylinder(q, r, r, dist, 12, 1); 
    glPopMatrix();
}
//rendering
void render_neuron(Neuron* n) {
  glEnable(GL_LIGHTING);

    GLfloat emission_off[]   = {0.0f, 0.0f, 0.0f, 1.0f}; 
    for (int i = 0; i < n->n_comparts; i++) {
        if (n->parent[i] == -1) continue;
        int p = n->parent[i]; // parent is already 0-based

        double t = n->vis_trace[i];
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        GLfloat emission_level[] = {
            (GLfloat)(t * 1.0),
            (GLfloat)(t * 0.9),
            (GLfloat)(t * 0.1),
            1.0f
        };
        glMaterialfv(GL_FRONT, GL_EMISSION, emission_level);
        glColor3f((GLfloat)(0.55 + 0.45 * t), (GLfloat)(0.68 + 0.32 * t), (GLfloat)(0.90 - 0.15 * t));

        draw_cylinder(n->x[i], n->y[i], n->z[i], 
                  n->x[p], n->y[p], n->z[p], 
                  n->r[i], n->quadric);
    }
    
    //glow effect for soma
    int s_id = n->soma_id;
    glPushMatrix();
        glTranslated(n->x[s_id], n->y[s_id], n->z[s_id]);
        double ts = n->vis_trace[s_id];
        if (ts < 0.0) ts = 0.0;
        if (ts > 1.0) ts = 1.0;
        GLfloat soma_emission[] = {
            (GLfloat)(ts * 1.0),
            (GLfloat)(ts * 0.9),
            (GLfloat)(ts * 0.2),
            1.0f
        };
        glMaterialfv(GL_FRONT, GL_EMISSION, soma_emission);
        glColor3f((GLfloat)(0.70 + 0.30 * ts), (GLfloat)(0.78 + 0.22 * ts), (GLfloat)(0.96 - 0.16 * ts));
        gluSphere(n->quadric, n->r[s_id] * 1.2, 32, 32);
    glPopMatrix();

    glMaterialfv(GL_FRONT, GL_EMISSION, emission_off);
    glDisable(GL_LIGHTING);

    // light overlay for recent activity
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < n->n_comparts; i++) {
        if (n->parent[i] == -1) continue;
        int p = n->parent[i];
        float t = (float)n->vis_trace[i];
        if (t < 0.05f) continue;

        glLineWidth(2.5f + 7.0f * t);
        glColor4f(1.0f, 0.95f, 0.35f, 0.40f + 0.60f * t);
        glBegin(GL_LINES);
            glVertex3d(n->x[p], n->y[p], n->z[p]);
            glVertex3d(n->x[i], n->y[i], n->z[i]);
        glEnd();
    }
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void render_synapse(Synapse* s) {
    Neuron* pre = &neurons[s->pre_neuron_id];
    Neuron* post = &neurons[s->post_neuron_id];

    
    int pre_idx = s->pre_compart;   //  terminal axonal compartment of pre-synaptic neuron
    int post_idx = s->post_compart; // contact point (soma) of post-synaptic neuron

    //adjust weight 
    glLineWidth((float)(1.5 + s->weight * 2.0)); 
    
    //orange synapse color
    glColor3f(1.0f, 0.5f, 0.0f); 

    glBegin(GL_LINES);
        glVertex3d(pre->x[pre_idx], pre->y[pre_idx], pre->z[pre_idx]);
        glVertex3d(post->x[post_idx], post->y[post_idx], post->z[post_idx]);
    glEnd();

    glPushMatrix();
        glTranslated(post->x[post_idx], post->y[post_idx], post->z[post_idx]);
        glutSolidSphere(1.0 + s->weight * 5.0, 10, 10);    // sphere size is based on synaptic weight to enhance visibility
    glPopMatrix();

    glLineWidth(1.0f); 
    glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white for other elements
} 

void draw_ui_plots() {
    float y_min = 0.0f;
    float y_max = 1.0f;
    int has_samples = 0;
    const float time_window_ms = (HISTORY_SIZE - 1) * PLOT_TIME_STEP_MS;
    float time_end_ms = (float)sim_time;
    float time_start_ms = time_end_ms - time_window_ms;

    if (time_start_ms < 0.0f) {
        time_start_ms = 0.0f;
    }

    for (int n = 0; n < n_neurons; n++) {
        for (int j = 0; j < HISTORY_SIZE; j++) {
            int idx = (history_index + j) % HISTORY_SIZE;
            float v = stim_history[n][idx];
            if (!has_samples) {
                y_min = v;
                y_max = v;
                has_samples = 1;
            } else {
                if (v < y_min) y_min = v;
                if (v > y_max) y_max = v;
            }
        }
    }

    if (!has_samples) {
        y_min = 0.0f;
        y_max = 1.0f;
    }

    {
        float y_range = y_max - y_min;
        if (y_range < 1e-6f) {
            y_min -= 0.5f;
            y_max += 0.5f;
        } else {
            float pad = y_range * PLOT_Y_PADDING_FACTOR;
            y_min -= pad;
            y_max += pad;
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING); 
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1024, 0, 768); // coordinate system for 2D overlay
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(0.2f, 0.3f, 0.4f, 0.7f);
    glBegin(GL_QUADS);
        glVertex2f(PLOT_LEFT - 10.0f, PLOT_TOP + 10.0f);
        glVertex2f(PLOT_RIGHT + 10.0f, PLOT_TOP + 10.0f);
        glVertex2f(PLOT_RIGHT + 10.0f, PLOT_BOTTOM - 10.0f);
        glVertex2f(PLOT_LEFT - 10.0f, PLOT_BOTTOM - 10.0f);
    glEnd();

    glColor3f(0.85f, 0.9f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex2f(PLOT_LEFT, PLOT_BOTTOM);
        glVertex2f(PLOT_RIGHT, PLOT_BOTTOM);
        glVertex2f(PLOT_LEFT, PLOT_BOTTOM);
        glVertex2f(PLOT_LEFT, PLOT_TOP);
    glEnd();

    glLineWidth(1.0f);
    draw_text(PLOT_RIGHT - 70.0f, PLOT_BOTTOM - 18.0f, "Time");
    draw_text(PLOT_LEFT - 55.0f, PLOT_TOP + 5.0f, "Impulse");

    for (int tick = 0; tick <= 4; tick++) {
        float x = PLOT_LEFT + (PLOT_RIGHT - PLOT_LEFT) * (tick / 4.0f);
        char label[16];
        float t = time_start_ms + (time_end_ms - time_start_ms) * (tick / 4.0f);
        snprintf(label, sizeof(label), "%.1f", t);
        glBegin(GL_LINES);
            glVertex2f(x, PLOT_BOTTOM);
            glVertex2f(x, PLOT_BOTTOM - 5.0f);
        glEnd();
        draw_text(x - 8.0f, PLOT_BOTTOM - 18.0f, label);
    }

    for (int tick = 0; tick <= 3; tick++) {
        float y = PLOT_BOTTOM + (PLOT_TOP - PLOT_BOTTOM) * (tick / 3.0f);
        float value = y_min + (y_max - y_min) * (tick / 3.0f);
        char label[16];
        snprintf(label, sizeof(label), "%.1f", value);
        glBegin(GL_LINES);
            glVertex2f(PLOT_LEFT, y);
            glVertex2f(PLOT_LEFT - 5.0f, y);
        glEnd();
        draw_text(PLOT_LEFT - 40.0f, y - 3.0f, label);
    }
    
    for (int n = 0; n < n_neurons; n++) {
        glColor3f(0.8f, 0.45f, 0.15f); 
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < HISTORY_SIZE; j++) {
            int idx = (history_index + j) % HISTORY_SIZE;
            float x = PLOT_LEFT + (PLOT_RIGHT - PLOT_LEFT) * (j / (float)(HISTORY_SIZE - 1));
            float value = stim_history[n][idx];
            if (value < y_min) value = y_min;
            if (value > y_max) value = y_max;
            float y = PLOT_BOTTOM + (value - y_min) * (PLOT_TOP - PLOT_BOTTOM) / (y_max - y_min);
            glVertex2f(x, y);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}
