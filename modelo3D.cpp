/*
 * Computacao Grafica
 * Jogo: Megaman
 * Autores: Prof. Iarhel Sabóia e Jaime Alves
 */

/* Inclui os headers do OpenGL, GLU, e GLUT */
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

/* Inclui o arquivo header com a textura */
#include "glm.h"
#include "glut_text.h"
#include <cstdio>
#include <cmath>
#include <map>
#include <vector>
#include <ctime>
#include <windows.h>
#include "MMSystem.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
// ------------------------------------------------------------Início do Código------------------------------------------------------------

// Tamanho da sala
const float roomSize = 1.0f;
const float wallHeight = 0.5f;
const float wallThickness = 0.02f;

// Posição do Cut Man
float cutManX = roomSize / 2 - 0.1f;  // Posicionado no canto direito
float cutManY = -0.2f;                  // Sobre o chão
float cutManZ = 0.0f;
float cutmanMovementSpeed = 0.2f; // Aumente para tornar o Cutman mais ágil
float cutmanLastMoveTime = 0.0f;
float cutmanMoveInterval = 2.0f; // Intervalo de 2 segundos para movimentação
bool cutmanJumping = false;
float cutmanJumpVelocity = 0.0f;
float cutmanGravity = 0.01f; // Ajuste conforme necessário
float cutmanJumpHeight = 0.2f; // Altura máxima do pulo
float cutmanJumpDuration = 1.0f; // Duração do pulo
float cutmanJumpStartTime = 0.0f;

// Posição do Mega Man
float megaManX = -roomSize / 2 + 0.1f; // Posicionado no canto esquerdo
float megaManY = -0.2f;                  // Sobre o chão
float megaManZ = 0.0f;
float lastDirectionX = 0.0f;

// Dimensões dos cubos (Cut Man e Mega Man)
const float cubeSize = 0.1f;

// Velocidade do Mega Man
float speedX = 0.01f;
float jumpSpeed = 0.02f;
float gravity = 0.001f;
float velocityY = 0.0f;

// Estado do Mega Man
bool isJumping = false;
bool onGround = true;
bool moveLeft = false;
bool moveRight = false;

// Pontos de vida dos personagens
int megaManHealth = 8;
int cutManHealth = 8;

// Variáveis de invulnerabilidade
bool megaManInvulnerable = false;
bool cutManInvulnerable = false;
float invulnerableTime = 0.0f;
const float invulnerableDuration = 1.0f; // 1 segundo

// Estruturas para personagens e projéteis
struct Projectile {
    float x, y, z;
    float dx, dy, dz;
    bool active;
    float creationTime;
};

std::vector<Projectile> projectiles;
const float projectileSize = 0.05f; // Ajuste o tamanho conforme necessário

struct Character {
    float x, y, z;
    float width, height, depth;
    float health;
    bool invulnerable;               // Adicionado para controlar o estado de invulnerabilidade
    float invulnerableStartTime;    // Adicionado para controlar o tempo de início da invulnerabilidade
};

Character megaman = { -roomSize / 2 + 0.1f, -0.2f, 0.0f, cubeSize, cubeSize, cubeSize, megaManHealth, false, 0.0f };
Character cutman = { roomSize / 2 - 0.1f, -0.2f, 0.0f, cubeSize, cubeSize * 2.0f, cubeSize, cutManHealth, false, 0.0f };


// Função para desenhar a barra de vida
void drawHealthBar(float x, float y, float z, int health) {
    const float barWidth = 0.2f;
    const float barHeight = 0.02f;
    const float segmentWidth = barWidth / 8.0f; // 8 pontos de vida

    glPushMatrix();
    glTranslatef(x, y, z);

    glDisable(GL_LIGHTING); // Desabilita a iluminação para as barras de vida

    for (int i = 0; i < 8; ++i) {
        if (i < health) {
            glColor3f(0.0f, 1.0f, 0.0f); // Verde para vida restante
        } else {
            glColor3f(1.0f, 0.0f, 0.0f); // Vermelho para vida perdida
        }
        glBegin(GL_QUADS);
        glVertex3f(i * segmentWidth, 0.0f, 0.0f);
        glVertex3f((i + 1) * segmentWidth, 0.0f, 0.0f);
        glVertex3f((i + 1) * segmentWidth, barHeight, 0.0f);
        glVertex3f(i * segmentWidth, barHeight, 0.0f);
        glEnd();
    }

    glEnable(GL_LIGHTING); // Habilita a iluminação novamente

    glPopMatrix();
}
// ------------------------------------------------------------Áudio------------------------------------------------------------

void playSound(const char* soundFile) {
    PlaySound(TEXT(soundFile), NULL, SND_ASYNC | SND_FILENAME);
}

void land() {
    playSound("MegamanLand.wav");
}

void shoot() {
    playSound("MegaBuster.wav");
}

void damage() {
    playSound("MegamanDamage.wav");
}

// ------------------------------------------------------------Colisão------------------------------------------------------------
bool checkCollision(float px, float py, float pz, float pSize,
                    float cx, float cy, float cz, float cSize) {
    // Calcula o ponto mais próximo do cubo para o projétil
    float closestX = std::max(cx - cSize / 2, std::min(px, cx + cSize / 2));
    float closestY = std::max(cy - cSize / 2, std::min(py, cy + cSize / 2));
    float closestZ = std::max(cz - cSize / 2, std::min(pz, cz + cSize / 2));
    
    // Calcula a distância entre o ponto mais próximo e o projétil
    float distanceX = px - closestX;
    float distanceY = py - closestY;
    float distanceZ = pz - closestZ;
    
    // Verifica se a distância é menor que o raio da esfera
    return (distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ) < (pSize * pSize);
}


// ------------------------------------------------------------Projéteis------------------------------------------------------------

void drawProjectile(const Projectile& projectile) {
    if (projectile.active) {
            glPushMatrix();
            glTranslatef(projectile.x, projectile.y, projectile.z);
            glColor3f(0.0f, 1.0f, 0.0f); // Cor verde para o projétil
            glutWireSphere(projectileSize, 10, 10); // Desenha a esfera com o tamanho ajustado
            glPopMatrix();
	}
}

void fireProjectile() {
    Projectile projectile;
    projectile.x = megaManX;
    projectile.y = megaManY;
    projectile.z = megaManZ;
    projectile.dx = lastDirectionX * 0.2f; // Direção do projétil
    projectile.dy = 0.0f;
    projectile.active = true;
    projectile.creationTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    
    projectiles.push_back(projectile);
}

void checkProjectileCollisions() {
    for (std::vector<Projectile>::iterator it = projectiles.begin(); it != projectiles.end(); ) {
        Projectile& projectile = *it;

        if (!projectile.active) {
            it = projectiles.erase(it);
            continue;
        }

        // Verifica colisão com o Cutman
        if (checkCollision(projectile.x, projectile.y, projectile.z, projectileSize, 
                           cutman.x, cutman.y, cutman.z, cutman.width)) {
            if (!cutman.invulnerable) {
                cutman.health--; // Aplica dano ao Cutman
                cutman.invulnerable = true;
                cutman.invulnerableStartTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            }
            projectile.active = false;
            it = projectiles.erase(it); // Remove o projétil da lista
        } else {
            // Verifica colisão com as paredes
            if (fabs(projectile.x) > roomSize / 2 || fabs(projectile.y) > roomSize / 2 || fabs(projectile.z) > roomSize / 2) {
                projectile.active = false;
                it = projectiles.erase(it); // Remove o projétil da lista
            } else {
                ++it; // Move para o próximo projétil se não houver colisão
            }
        }
    }
}


void updateProjectiles() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Tempo atual em segundos

    for (size_t i = 0; i < projectiles.size(); ) {
        Projectile& projectile = projectiles[i];
        
        if (!projectile.active) {
            projectiles.erase(projectiles.begin() + i); // Remove o projétil da lista
            continue;
        }

        // Atualiza a posição do projétil
        projectile.x += projectile.dx;
        projectile.y += projectile.dy;
        projectile.z += projectile.dz;

        // Verifica colisão com o Cutman
        if (checkCollision(projectile.x, projectile.y, projectile.z, projectileSize, 
                           cutman.x, cutman.y, cutman.z, cutman.width)) {
            if (!cutman.invulnerable) {
                cutman.health--; // Aplica dano ao Cutman
                cutman.invulnerable = true;
                cutman.invulnerableStartTime = currentTime;
            }
            projectile.active = false; // Desativa o projétil
            projectiles.erase(projectiles.begin() + i); // Remove o projétil da lista
            continue;
        }

        // Verifica colisão com as paredes
        if (fabs(projectile.x) > roomSize / 2 || fabs(projectile.y) > roomSize / 2 || fabs(projectile.z) > roomSize / 2) {
            projectile.active = false;
            projectiles.erase(projectiles.begin() + i); // Remove o projétil da lista
            continue;
        }

        ++i;
    }
}



void drawProjectiles() {
    for (size_t i = 0; i < projectiles.size(); ++i) {
        const Projectile& projectile = projectiles[i];
        if (projectile.active) {
            glPushMatrix();
            glTranslatef(projectile.x, projectile.y, projectile.z);

            // Define a cor verde para o projétil
            glColor3f(0.0f, 1.0f, 0.0f); // Verde

            // Desenha o projétil com tamanho reduzido
            glutSolidSphere(0.02, 10, 10); // Tamanho reduzido
            glPopMatrix();
        }
    }
}



// ------------------------------------------------------------Cutman------------------------------------------------------------

void moveCutman() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // Verifica se é hora de mudar a direção ou pular
    if (currentTime - cutmanLastMoveTime > cutmanMoveInterval) {
        cutmanLastMoveTime = currentTime;

            float dx = megaManX - cutManX;
            float dz = megaManZ - cutManZ;
            float distance = sqrt(dx * dx + dz * dz);

            // Normaliza o vetor de direção
            if (distance > 0.0f) {
                dx /= distance;
                dz /= distance;
            }

            // Atualiza a posição do Cutman com interpolação suave
            cutManX += dx * cutmanMovementSpeed;
            cutManZ += dz * cutmanMovementSpeed;

            // Verifica os limites da sala
            if (cutManX > roomSize / 2 - cubeSize / 2) cutManX = roomSize / 2 - cubeSize / 2;
            if (cutManX < -roomSize / 2 + cubeSize / 2) cutManX = -roomSize / 2 + cubeSize / 2;
            if (cutManZ > roomSize / 2 - cubeSize / 2) cutManZ = roomSize / 2 - cubeSize / 2;
            if (cutManZ < -roomSize / 2 + cubeSize / 2) cutManZ = -roomSize / 2 + cubeSize / 2;
    }
}


void drawCutMan() {
    glPushMatrix();
    glTranslatef(cutManX, cutManY, cutManZ);
	glDisable(GL_LIGHTING);
    // Corpo de Cut Man
    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidCube(cubeSize);

    // Lâmina de Cut Man (rotacionada em 90 graus no eixo Y)
    glPushMatrix();
    glTranslatef(0.0f, 0.05f, 0.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); // Rotaciona a lâmina em 90 graus no eixo Y
    glBegin(GL_QUADS);
        glVertex3f(-0.05f, -0.01f, 0.0f);
        glVertex3f( 0.05f, -0.01f, 0.0f);
        glVertex3f( 0.05f,  0.01f, 0.0f);
        glVertex3f(-0.05f,  0.01f, 0.0f);
    glEnd();
    glPopMatrix();

    glPopMatrix();
}

// ------------------------------------------------------------Inicialização da cena------------------------------------------------------------

// Função para inicializar a cena
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Configuração da luz
    GLfloat light_position[] = { 0.0f, 1.0f, 0.0f, 0.0f }; // Luz direcional vinda de cima
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Cor da luz difusa (branca)
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Cor da luz especular (branca)
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Cor da luz ambiente (fraca)

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

    // Material dos objetos
    GLfloat mat_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat mat_shininess[] = { 0.0f };
    GLfloat mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_ambient[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Cor de fundo
}

// ------------------------------------------------------------Megaman------------------------------------------------------------

// Função para desenhar o Mega Man
void drawMegaMan() {
    glPushMatrix();
    glTranslatef(megaManX, megaManY, megaManZ);

	glDisable(GL_LIGHTING);
    // Corpo do Mega Man
    if (isJumping) {
        glColor3f(0.0f, 0.0f, 1.0f); // Azul do Mega Man
    } else {
        glColor3f(0.0f, 0.0f, 1.0f); // Azul do Mega Man
    }
    glutSolidCube(cubeSize);

    glPopMatrix();
}

// Função de teclado para movimentação
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'd':
            moveRight = true; // Move para a direita
            lastDirectionX = 1.0f; // Direção para a direita
            break;
        case 'a':
            moveLeft = true; // Move para a esquerda
            lastDirectionX = -1.0f; // Direção para a esquerda
            break;
        case ' ':
            if (onGround) {
                isJumping = true;
                land();
                onGround = false;
                velocityY = jumpSpeed; // Começa o pulo
            }
            break;
        case 'j':
            fireProjectile();
            shoot();
            break;
        default:
            break;
    }
}


// Função para detectar quando as teclas são soltas
void keyboardUp(unsigned char key, int x, int y) {
    switch (key) {
        case 'd':
            moveRight = false; // Para de mover para a direita
            lastDirectionX = 1.0f; // Direção para a direita
            break;
        case 'a':
            moveLeft = false; // Para de mover para a esquerda
            lastDirectionX = -1.0f; // Direção para a esquerda
            break;
        default:
            break;
    }
}

// ------------------------------------------------------------Desenho da Sala------------------------------------------------------------

void drawWall(float x, float y, float z, float width, float height, float depth) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(width, height, depth);
    glutSolidCube(1.0f);
    glPopMatrix();
}


// Função para desenhar a sala
void drawRoom() {
    // Desenha o chão
    glPushMatrix();
    glColor3f(0.9f, 0.9f, 0.2f); // Amarelo claro
    glTranslatef(0.0f, -wallHeight / 2, 0.0f);
    glScalef(roomSize, wallThickness, roomSize);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Desenha as paredes laterais
    glColor3f(0.8f, 0.8f, 0.0f); // Amarelo escuro
    // Parede esquerda
    glPushMatrix();
    glTranslatef(-roomSize / 2 + wallThickness / 2, 0.0f, 0.0f);
    glScalef(wallThickness, wallHeight, roomSize);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Parede direita
    glPushMatrix();
    glTranslatef(roomSize / 2 - wallThickness / 2, 0.0f, 0.0f);
    glScalef(wallThickness, wallHeight, roomSize);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Parede do fundo
    glColor3f(0.7f, 0.7f, 0.0f); // Amarelo mais escuro
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -roomSize / 2 + wallThickness / 2);
    glScalef(roomSize, wallHeight, wallThickness);
    glutSolidCube(1.0f);
    glPopMatrix();
}

// Função de display
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Posiciona a câmera de frente para a sala
    gluLookAt(0.0, 0.0, 1.5,  // Posição da câmera
              0.0, 0.0, 0.0,  // Para onde a câmera está olhando
              0.0, 1.0, 0.0); // Vetor "up"
              
    drawRoom();
    drawCutMan();
    drawMegaMan();
    drawProjectiles(); // Certifique-se de que essa função está sendo chamada
    drawHealthBar(-roomSize / 2 + 0.05f, wallHeight + 0.05f, 0.0f, megaManHealth); // Barra de vida do Mega Man
    drawHealthBar(roomSize / 2 - 0.25f, wallHeight + 0.05f, 0.0f, cutManHealth);  // Barra de vida do Cut Man
    
    glutSwapBuffers();
}


void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}



// Função para atualizar a posição do Mega Man e a lógica do pulo
void update(int value) {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Tempo atual em segundos

    // Atualiza a posição do Cutman e projéteis
    moveCutman();
    updateProjectiles(); // Atualiza a posição dos projéteis e verifica colisões

    // Movimenta o Mega Man
    if (moveRight && megaManX + cubeSize / 2 < roomSize / 2 - wallThickness) {
        megaManX += speedX;
    }
    if (moveLeft && megaManX - cubeSize / 2 > -roomSize / 2 + wallThickness) {
        megaManX -= speedX;
    }

    // Lógica do pulo
    if (isJumping) {
        megaManY += velocityY;
        velocityY -= gravity;

        if (megaManY <= -0.2f) { // Chega ao chão
            megaManY = -0.2f;
            isJumping = false;
            onGround = true;
            land();
            velocityY = 0.0f;
        }
    } else if (!onGround) { // Em queda livre
        megaManY += velocityY;
        velocityY -= gravity;

        if (megaManY <= -0.2f) { // Chega ao chão
            megaManY = -0.2f;
            onGround = true;
            land();
            velocityY = 0.0f;
        }
    }
	// Verifica colisões dos projéteis com o Cutman e as paredes
    checkProjectileCollisions();
    // Verifica colisão entre Mega Man e Cut Man
    if (checkCollision(megaManX, megaManY, megaManZ, cubeSize, cutManX, cutManY, cutManZ, cubeSize)) {
        if (!megaManInvulnerable) {
            megaManHealth--;
            damage();
            // Aplica pushback
            if (megaManX < cutManX) {
                megaManX -= 0.1f; // Move para a esquerda
            } else {
                megaManX += 0.1f; // Move para a direita
            }
            megaManInvulnerable = true;
            invulnerableTime = currentTime; // Atualiza o tempo de invulnerabilidade
        }
    }

    // Verifica o tempo de invulnerabilidade do Mega Man
    if (megaManInvulnerable) {
        if (currentTime - invulnerableTime > invulnerableDuration) {
            megaManInvulnerable = false;
        }
    }

    // Restringe a posição do Mega Man dentro da sala
    if (megaManX + cubeSize / 2 > roomSize / 2 - wallThickness) {
        megaManX = roomSize / 2 - wallThickness - cubeSize / 2;
    } else if (megaManX - cubeSize / 2 < -roomSize / 2 + wallThickness) {
        megaManX = -roomSize / 2 + wallThickness + cubeSize / 2;
    }

    // Restringe a posição do Cut Man dentro da sala
    if (cutManX + cubeSize / 2 > roomSize / 2 - wallThickness) {
        cutManX = roomSize / 2 - wallThickness - cubeSize / 2;
    } else if (cutManX - cubeSize / 2 < -roomSize / 2 + wallThickness) {
        cutManX = -roomSize / 2 + wallThickness + cubeSize / 2;
    }
    
    // Atualiza o estado do Cutman
    if (cutman.invulnerable) {
        if (currentTime - cutman.invulnerableStartTime > invulnerableDuration) {
            cutman.invulnerable = false;
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // Atualiza a cada 16 ms (aproximadamente 60 FPS)
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Sala do Cut Man - Mega Man");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutTimerFunc(25, update, 0); // Inicia o timer para atualização do pulo
    glutMainLoop();
    return 0;
}
