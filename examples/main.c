#include "gl_platform.h"

#include "math3d.h"
#include "petal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PETAL_OUTWARD 0.44f
#define PETAL_SCALE_LARGE 1.10f
#define PETAL_SCALE_SMALL 0.84f
#define CAM_DIST_DEFAULT 5.2f
#define CAM_DIST_MIN 1.6f
#define CAM_DIST_MAX 16.f

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

static int g_width = 960;
static int g_height = 720;
static int g_perspective = 1;
static int g_wire = 0;
static int g_smooth = 1;
static int g_lighting = 1;
static int g_depth = 1;
static int g_spin = 0;
static float g_yaw = -32.f;
static float g_pitch = 26.f;
static float g_cam_dist = CAM_DIST_DEFAULT;
static float g_petal_scale = 1.f;
static int g_dragging = 0;
static int g_last_x = 0;
static int g_last_y = 0;

/* Pegamos esses tons da foto da logo. */
static const float kOrangeDark[3] = {215.f / 255.f, 126.f / 255.f, 42.f / 255.f};
static const float kOrangeLight[3] = {234.f / 255.f, 168.f / 255.f, 85.f / 255.f};
static const float kGreenDark[3] = {96.f / 255.f, 134.f / 255.f, 72.f / 255.f};
static const float kGreenLight[3] = {150.f / 255.f, 186.f / 255.f, 118.f / 255.f};

static void idle(void);
static void refresh_title(void);

static void refresh_title(void)
{
    char title[160];

    snprintf(title, sizeof title, "e-colab | %s | Gouraud %s | luz %s | z %s",
             g_perspective ? "perspectiva" : "ortogonal",
             g_smooth ? "on" : "off",
             g_lighting ? "on" : "off",
             g_depth ? "on" : "off");
    glutSetWindowTitle(title);
}

static void setup_lighting(void)
{
    float global_ambient[] = {0.46f, 0.46f, 0.44f, 1.f};
    float ambient[] = {0.22f, 0.22f, 0.20f, 1.f};
    float diffuse[] = {0.88f, 0.88f, 0.84f, 1.f};
    float specular[] = {0.28f, 0.28f, 0.26f, 1.f};
    float mat_specular[] = {0.22f, 0.22f, 0.20f, 1.f};

    /* Phong pela OpenGL. Na tecla G trocamos Gouraud (SMOOTH) por FLAT. */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.f);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
}

static void draw_petals(void)
{
    /* Quatro cópias da mesma pétala: laranja em cima, verde embaixo. */
    const float degrees[] = {-45.f, 45.f, 135.f, 225.f};
    int i;

    for (i = 0; i < 4; i++) {
        float radians = degrees[i] * MATH3D_PI / 180.f;
        float rz[16];
        float t[16];
        float s[16];
        int upper = cosf(radians) > 0.f;
        /* Na referência uma diagonal é grande e a outra pequena. */
        int large = sinf(2.f * radians) > 0.f;
        float petal_size = g_petal_scale * (large ? PETAL_SCALE_LARGE : PETAL_SCALE_SMALL);
        float out = PETAL_OUTWARD * (large ? 1.04f : 0.90f);

        /* Escalamos, empurramos no +Y e giramos em Z. Ordem: v' = Rz * T * S * v. */
        glPushMatrix();
        mat4_make_rotation_z(rz, radians);
        apply_matrix(rz);
        mat4_make_translation(t, 0.f, out, 0.f);
        apply_matrix(t);
        mat4_make_scale(s, petal_size, petal_size, petal_size);
        apply_matrix(s);
        if (upper) {
            petal_draw(kOrangeDark, kOrangeLight);
        } else {
            petal_draw(kGreenDark, kGreenLight);
        }
        glPopMatrix();
    }
}

static void display(void)
{
    float proj[16];
    float aspect = (float)g_width / (float)g_height;
    float cam[16];
    float rx[16];
    float ry[16];
    float light_pos[] = {1.8f, 2.4f, 5.0f, 1.f};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, g_width, g_height);
    glPolygonMode(GL_FRONT_AND_BACK, g_wire ? GL_LINE : GL_FILL);
    glShadeModel(g_smooth ? GL_SMOOTH : GL_FLAT);
    /* Tecla Z: visibilidade. Sem o teste, o último desenho tapa o da frente. */
    if (g_depth) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (g_lighting) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* Teclas P e O: as duas projeções, com a matriz que montamos. */
    if (g_perspective) {
        mat4_make_perspective(proj, 40.f * MATH3D_PI / 180.f, aspect, 0.1f, 80.f);
    } else {
        float half = 0.45f * g_cam_dist;
        mat4_make_ortho(proj, -half * aspect, half * aspect, -half, half, -20.f, 20.f);
    }
    apply_matrix(proj);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    mat4_make_translation(cam, 0.f, 0.18f, -g_cam_dist);
    apply_matrix(cam);

    /* Colocamos a luz antes de orbitar, senão ela gira junto com o modelo. */
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    mat4_make_rotation_x(rx, g_pitch * MATH3D_PI / 180.f);
    apply_matrix(rx);
    mat4_make_rotation_y(ry, g_yaw * MATH3D_PI / 180.f);
    apply_matrix(ry);

    draw_petals();

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h < 1) {
        h = 1;
    }
    g_width = w;
    g_height = h;
    glutPostRedisplay();
}

static void idle(void)
{
    g_yaw += 0.35f;
    if (g_yaw > 180.f) {
        g_yaw -= 360.f;
    }
    glutPostRedisplay();
}

static void sync_spin(void)
{
    glutIdleFunc(g_spin ? idle : NULL);
}

static void print_matrices(void)
{
    float s[16];
    float t[16];
    float r[16];
    float ts[16];
    float rts[16];
    float x;
    float y;
    float z;
    float radians = 45.f * MATH3D_PI / 180.f;

    printf("\n--- conferencia das matrizes (escala atual %.2f) ---\n", g_petal_scale);
    printf("Conferir a mao: ponto local (0.20, 0, 0), depois S, T e Rz de 45 graus.\n");

    mat4_make_scale(s, g_petal_scale, g_petal_scale, g_petal_scale);
    mat4_make_translation(t, 0.f, PETAL_OUTWARD, 0.f);
    mat4_make_rotation_z(r, radians);
    mat4_multiply(ts, t, s);
    mat4_multiply(rts, r, ts);
    mat4_transform_point(rts, 0.20f, 0.f, 0.f, &x, &y, &z);

    mat4_print("S", s);
    mat4_print("T", t);
    mat4_print("Rz(45 graus)", r);
    mat4_print("Rz * T * S", rts);
    printf("Rz*T*S * (0.20, 0, 0) = (%.3f, %.3f, %.3f)\n", x, y, z);
    printf("Com escala 1:    aproximadamente (-0.297, 0.580, 0)\n");
    printf("Com escala 1.2:  aproximadamente (-0.269, 0.608, 0)\n\n");
}

static void clamp_camera(void)
{
    if (g_cam_dist < CAM_DIST_MIN) {
        g_cam_dist = CAM_DIST_MIN;
    }
    if (g_cam_dist > CAM_DIST_MAX) {
        g_cam_dist = CAM_DIST_MAX;
    }
    if (g_pitch > 89.f) {
        g_pitch = 89.f;
    }
    if (g_pitch < -89.f) {
        g_pitch = -89.f;
    }
}

static void zoom_by(float delta)
{
    g_cam_dist += delta;
    clamp_camera();
}

static void set_view(float yaw, float pitch, int reset_zoom)
{
    g_yaw = yaw;
    g_pitch = pitch;
    g_spin = 0;
    sync_spin();
    if (reset_zoom) {
        g_cam_dist = CAM_DIST_DEFAULT;
    }
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    switch (key) {
    case 'p':
    case 'P':
        g_perspective = 1;
        printf("Projecao perspectiva.\n");
        break;
    case 'o':
    case 'O':
        g_perspective = 0;
        printf("Projecao ortogonal. Gire o modelo: o tamanho nao muda com a profundidade.\n");
        break;
    case 'f':
    case 'F':
    case '1':
        set_view(0.f, 0.f, 0);
        printf("Vista frontal, para comparar com a referencia.\n");
        break;
    case '2':
        set_view(-32.f, 26.f, 0);
        printf("Vista 3/4.\n");
        break;
    case '3':
        set_view(90.f, 8.f, 0);
        printf("Vista de lado. O tubo das petalas aparece em volume.\n");
        break;
    case '4':
        set_view(0.f, 80.f, 0);
        printf("Vista de cima.\n");
        break;
    case 'r':
    case 'R':
        set_view(-32.f, 26.f, 1);
        printf("Camera reiniciada.\n");
        break;
    case ']':
        zoom_by(-0.35f);
        printf("Zoom in, distancia %.2f\n", g_cam_dist);
        break;
    case '[':
        zoom_by(0.35f);
        printf("Zoom out, distancia %.2f\n", g_cam_dist);
        break;
    case 'w':
    case 'W':
        g_wire = !g_wire;
        printf("Malha %s.\n", g_wire ? "aramada" : "preenchida");
        break;
    case 'g':
    case 'G':
        g_smooth = !g_smooth;
        printf("Tonalizacao %s.\n", g_smooth ? "Gouraud (GL_SMOOTH)" : "constante (GL_FLAT)");
        break;
    case 'l':
    case 'L':
        g_lighting = !g_lighting;
        printf("Iluminacao %s.\n", g_lighting ? "ligada" : "desligada");
        break;
    case 'z':
    case 'Z':
        g_depth = !g_depth;
        printf("Z-buffer %s. Gire com as setas: sem o teste, a petala desenhada por ultimo cobre a da frente.\n",
               g_depth ? "ligado" : "desligado");
        break;
    case ' ':
        g_spin = !g_spin;
        sync_spin();
        printf("Rotacao automatica %s.\n", g_spin ? "ligada" : "desligada");
        break;
    case '+':
    case '=':
        g_petal_scale += 0.05f;
        if (g_petal_scale > 2.2f) {
            g_petal_scale = 2.2f;
        }
        printf("Escala da petala: %.2f\n", g_petal_scale);
        break;
    case '-':
    case '_':
        g_petal_scale -= 0.05f;
        if (g_petal_scale < 0.3f) {
            g_petal_scale = 0.3f;
        }
        printf("Escala da petala: %.2f\n", g_petal_scale);
        break;
    case 'm':
    case 'M':
        print_matrices();
        break;
    case 27:
        exit(0);
        break;
    default:
        break;
    }

    refresh_title();
    glutPostRedisplay();
}

static void special_keys(int key, int x, int y)
{
    (void)x;
    (void)y;

    switch (key) {
    case GLUT_KEY_LEFT:
        g_yaw -= 4.f;
        break;
    case GLUT_KEY_RIGHT:
        g_yaw += 4.f;
        break;
    case GLUT_KEY_UP:
        g_pitch += 4.f;
        break;
    case GLUT_KEY_DOWN:
        g_pitch -= 4.f;
        break;
    case GLUT_KEY_PAGE_UP:
        zoom_by(-0.45f);
        break;
    case GLUT_KEY_PAGE_DOWN:
        zoom_by(0.45f);
        break;
    default:
        break;
    }

    clamp_camera();
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        g_dragging = (state == GLUT_DOWN);
        g_last_x = x;
        g_last_y = y;
    }
    if (button == 3 && state == GLUT_DOWN) {
        zoom_by(-0.28f);
        glutPostRedisplay();
    }
    if (button == 4 && state == GLUT_DOWN) {
        zoom_by(0.28f);
        glutPostRedisplay();
    }
}

static void motion(int x, int y)
{
    if (!g_dragging) {
        return;
    }
    g_yaw += (float)(x - g_last_x) * 0.35f;
    g_pitch += (float)(y - g_last_y) * 0.35f;
    g_last_x = x;
    g_last_y = y;
    clamp_camera();
    glutPostRedisplay();
}

static void mouse_wheel(int wheel, int direction, int x, int y)
{
    (void)wheel;
    (void)x;
    (void)y;
    zoom_by(direction > 0 ? -0.32f : 0.32f);
    glutPostRedisplay();
}

static void init_gl(void)
{
    glClearColor(0.97f, 0.96f, 0.94f, 1.f);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glShadeModel(GL_SMOOTH);
    setup_lighting();
}

static void print_help(void)
{
    printf("e-colab — protótipo OpenGL. Guia: docs/PLANO.md\n");
    printf("Arrastar  orbitam o modelo (ou setas)\n");
    printf("Roda / [ ] / PageUp PageDown   zoom\n");
    printf("1 ou F    vista frontal\n");
    printf("2         vista 3/4\n");
    printf("3         vista de lado\n");
    printf("4         vista de cima\n");
    printf("R         reinicia a camera\n");
    printf("P / O     perspectiva / ortogonal\n");
    printf("W         aramado / preenchido\n");
    printf("G         Gouraud / tonalizacao constante\n");
    printf("L         liga / desliga a luz\n");
    printf("Z         liga / desliga o Z-buffer\n");
    printf("+ / -     escala das petalas\n");
    printf("Espaco    rotacao automatica\n");
    printf("M         imprime matrizes no console\n");
    printf("Esc       sai\n");
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(g_width, g_height);
    glutCreateWindow("e-colab");
    init_gl();
    refresh_title();
    print_help();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMouseWheelFunc(mouse_wheel);
    glutMainLoop();
    return 0;
}
