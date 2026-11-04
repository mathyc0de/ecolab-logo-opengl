#include "gl_platform.h"

#include "math3d.h"
#include "petal.h"

#include <stdio.h>
#include <stdlib.h>

/* Distância do centro da logo até o centro da pétala, no +Y local. */
#define PETAL_OUTWARD 0.44f
/* Uma diagonal da referência é maior que a outra. */
#define PETAL_SCALE_LARGE 1.10f
#define PETAL_SCALE_SMALL 0.84f
/* A câmera fica no eixo Z e olha para a origem. Número maior, logo menor. */
#define CAM_DIST_DEFAULT 5.2f
#define CAM_DIST_MIN 1.6f
#define CAM_DIST_MAX 16.f

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

static int g_width = 960;
static int g_height = 720;

/* Cada inteiro abaixo é uma tecla da demonstração. */
static int g_perspective = 1; /* P perspectiva, O ortogonal */
static int g_wire = 0;        /* W: preenchido ou aramado */
static int g_smooth = 1;      /* G: Gouraud (GL_SMOOTH) ou constante (GL_FLAT) */
static int g_lighting = 1;    /* L */
static int g_depth = 1;       /* Z: teste de profundidade */
static int g_spin = 0;        /* espaço: gira sozinho */

/* Órbita. A cena abre em 3/4 para o volume aparecer. */
static float g_yaw = -32.f;
static float g_pitch = 26.f;
static float g_cam_dist = CAM_DIST_DEFAULT;
static float g_petal_scale = 1.f; /* teclas + e - */
static int g_dragging = 0;
static int g_last_x = 0;
static int g_last_y = 0;

/* Tons da foto, em 0..1, que é a faixa do glColor. */
static const float kOrangeDark[3] = {215.f / 255.f, 126.f / 255.f, 42.f / 255.f};
static const float kOrangeLight[3] = {234.f / 255.f, 168.f / 255.f, 85.f / 255.f};
static const float kGreenDark[3] = {96.f / 255.f, 134.f / 255.f, 72.f / 255.f};
static const float kGreenLight[3] = {150.f / 255.f, 186.f / 255.f, 118.f / 255.f};

/* A barra da janela mostra o estado de P, G, L e Z sem precisar do console. */
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

/*
 * Phong pela OpenGL: I = ambiente + difusa * max(N·L, 0) + especular * (N·H)^n.
 * Gouraud é o GL_SMOOTH, que interpola essa cor entre os vértices.
 * A tecla G troca para GL_FLAT e a faceta do tubo aparece.
 */
static void setup_lighting(void)
{
    float global_ambient[] = {0.46f, 0.46f, 0.44f, 1.f};
    float ambient[] = {0.22f, 0.22f, 0.20f, 1.f};
    float diffuse[] = {0.88f, 0.88f, 0.84f, 1.f};
    float specular[] = {0.28f, 0.28f, 0.26f, 1.f};
    float mat_specular[] = {0.22f, 0.22f, 0.20f, 1.f};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    /* A escala da pétala encolhe a normal. Isto devolve comprimento 1. */
    glEnable(GL_NORMALIZE);
    /* Sem COLOR_MATERIAL, glColor é ignorado e a logo fica cinza. */
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    /* O especular depende de onde a câmera está, não de uma direção fixa. */
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.f); /* o n da fórmula */
    /* Soma o brilho depois da cor, senão o laranja engole o especular. */
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
}

static void draw_petals(void)
{
    /* Duas de cima, laranja; duas de baixo, verde. A diagonal alterna o tamanho. */
    const float degrees[4] = {-45.f, 45.f, 135.f, 225.f};
    const int large[4] = {0, 1, 0, 1};
    int i;

    for (i = 0; i < 4; i++) {
        float size = g_petal_scale * (large[i] ? PETAL_SCALE_LARGE : PETAL_SCALE_SMALL);
        float out = PETAL_OUTWARD * (large[i] ? 1.04f : 0.90f);
        float rz[16];
        float t[16];
        float s[16];

        /*
         * glMultMatrixf faz M = M * A, então a última matriz aplicada
         * é a primeira que toca o vértice: v' = Rz * T * S * v.
         * O push isola esta cópia; sem o pop a próxima herda a rotação.
         */
        glPushMatrix();
        mat4_make_rotation_z(rz, degrees[i] * MATH3D_PI / 180.f);
        apply_matrix(rz);
        mat4_make_translation(t, 0.f, out, 0.f);
        apply_matrix(t);
        mat4_make_scale(s, size, size, size);
        apply_matrix(s);
        if (i < 2) {
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

    /* Limpamos cor e profundidade. Sem o segundo, o quadro anterior vaza no Z. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, g_width, g_height);
    glPolygonMode(GL_FRONT_AND_BACK, g_wire ? GL_LINE : GL_FILL);
    glShadeModel(g_smooth ? GL_SMOOTH : GL_FLAT);
    /* Tecla Z: sem o teste, a pétala desenhada por último cobre a da frente. */
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

    /* Projeção e câmera são matrizes diferentes. Esquecer o modo é o bug clássico. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* Teclas P e O. As duas matrizes estão em math3d.c; aqui só escolhemos. */
    if (g_perspective) {
        mat4_make_perspective(proj, 40.f * MATH3D_PI / 180.f, aspect, 0.1f, 80.f);
    } else {
        /* O retângulo cresce com a distância para o zoom valer na ortogonal também. */
        float half = 0.45f * g_cam_dist;
        mat4_make_ortho(proj, -half * aspect, half * aspect, -half, half, -20.f, 20.f);
    }
    apply_matrix(proj);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /* Afasta a câmera. O 0.18 sobe um pouco o olhar, para a logo não colar no chão. */
    mat4_make_translation(cam, 0.f, 0.18f, -g_cam_dist);
    apply_matrix(cam);

    /* Antes da órbita: a luz fica parada no mundo. Depois, ela giraria com a logo. */
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
    /* Altura 0 estouraria o aspect na projeção. */
    if (h < 1) {
        h = 1;
    }
    g_width = w;
    g_height = h;
    glutPostRedisplay();
}

/* Espaço liga isto. Só o yaw anda; a luz, parada, faz o brilho caminhar. */
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

/* Tecla M. A mesma conta de draw_petals, num ponto fixo, para conferir no papel. */
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

    mat4_make_scale(s, g_petal_scale, g_petal_scale, g_petal_scale);
    mat4_make_translation(t, 0.f, PETAL_OUTWARD, 0.f);
    mat4_make_rotation_z(r, 45.f * MATH3D_PI / 180.f);
    mat4_multiply(ts, t, s);
    mat4_multiply(rts, r, ts);
    mat4_transform_point(rts, 0.20f, 0.f, 0.f, &x, &y, &z);

    printf("\nEscala %.2f. Ponto (0.20, 0, 0) depois de S, T e Rz(45).\n", g_petal_scale);
    mat4_print("S", s);
    mat4_print("T", t);
    mat4_print("Rz", r);
    mat4_print("Rz * T * S", rts);
    printf("resultado (%.3f, %.3f, %.3f)\n\n", x, y, z);
}

static float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/* Pitch em ±89 evita a câmera virar de cabeça para baixo no polo. */
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

/* Delta positivo afasta. Na perspectiva isso diminui a logo; na ortogonal também,
   porque o retângulo de mat4_make_ortho acompanha g_cam_dist. */
static void zoom_by(float delta)
{
    g_cam_dist += delta;
    clamp_camera();
}

/* Uma vista pronta. Para a rotação automática, senão o yaw segue andando. */
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
    case 'p': /* perspectiva */
    case 'P':
        g_perspective = 1;
        break;
    case 'o': /* ortogonal */
    case 'O':
        g_perspective = 0;
        break;
    case 'f': /* frontal, para comparar com a foto */
    case 'F':
    case '1':
        set_view(0.f, 0.f, 0);
        break;
    case '2': /* 3/4, a vista em que o volume aparece */
        set_view(-32.f, 26.f, 0);
        break;
    case '3': /* lado: o funil das pétalas */
        set_view(90.f, 8.f, 0);
        break;
    case '4': /* de cima */
        set_view(0.f, 80.f, 0);
        break;
    case 'r':
    case 'R':
        set_view(-32.f, 26.f, 1);
        break;
    case ']':
        zoom_by(-0.35f);
        break;
    case '[':
        zoom_by(0.35f);
        break;
    case 'w': /* malha */
    case 'W':
        g_wire = !g_wire;
        break;
    case 'g': /* Gouraud ou constante */
    case 'G':
        g_smooth = !g_smooth;
        break;
    case 'l': /* Phong ligado ou cor chapada */
    case 'L':
        g_lighting = !g_lighting;
        break;
    case 'z': /* Z-buffer */
    case 'Z':
        g_depth = !g_depth;
        break;
    case ' ':
        g_spin = !g_spin;
        sync_spin();
        break;
    case '+':
    case '=':
        g_petal_scale = clampf(g_petal_scale + 0.05f, 0.3f, 2.2f);
        break;
    case '-':
    case '_':
        g_petal_scale = clampf(g_petal_scale - 0.05f, 0.3f, 2.2f);
        break;
    case 'm':
    case 'M':
        print_matrices();
        break;
    case 27:
        exit(0);
        break;
    default:
        return;
    }

    refresh_title();
    glutPostRedisplay();
}

/* Setas orbitam. Page Up/Down dão zoom, no mesmo eixo da roda do mouse. */
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

/* No GLUT a roda chega como botão 3 (aproxima) e 4 (afasta). */
static void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        g_dragging = (state == GLUT_DOWN);
        g_last_x = x;
        g_last_y = y;
    } else if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        zoom_by(button == 3 ? -0.28f : 0.28f);
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

static void init_gl(void)
{
    /* Fundo próximo do branco da referência. */
    glClearColor(0.97f, 0.96f, 0.94f, 1.f);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); /* fica o fragmento mais perto da câmera */
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
    /* GLUT_DEPTH cria o Z-buffer. Sem ele a tecla Z não tem o que desligar. */
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
    glutMainLoop();
    return 0;
}
