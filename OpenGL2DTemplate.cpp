#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <glut.h>
#include <string.h>
#include <math.h>
#include <vector>

struct Point {
	float x;
	float y;
};
enum GameState {
	SETUP,
	PLAYING,
	GAME_OVER_WIN,
	GAME_OVER_LOSS
};

std::vector<Point> obstacles;
std::vector<Point> collectibles;
std::vector<Point> powerups1;
std::vector<Point> powerups2;
std::vector<Point> backgroundStars;

void Display();
void drawObstacle(float x, float y);
void drawCollectible(float x, float y);
void drawPowerUp1(float x, float y);
void drawPowerUp2(float x, float y);
void drawPlayer();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void bezier(float t, int* p0, int* p1, int* p2, int* p3, int* result);
void drawTarget();
void timer(int value);
void resetGame();
void passiveMotion(int x, int y);
void initBackground();
void drawBackground();

GameState currentState = SETUP;

float cursorX = 0;
float cursorY = 0;

float playerX = 25;
float playerY = 125;
float playerRotation = 0;

bool isTargetMovingForward = true;

int gameTime = 90;
int score = 0;
int lives = 5;

int drawingMode = 0;
bool gameIsRunning = false;

const float PLAYER_RADIUS = 15.0f;
const float COLLECTIBLE_RADIUS = 20.0f;
const float OBSTACLE_RADIUS = 20.0f;
const float POWERUP_RADIUS = 20.0f;

const float TARGET_RADIUS = 20.0f;

bool isSpeedBoostActive = false;
int speedBoostDuration = 0;

bool isShieldActive = false;
int shieldDuration = 0;

float animationTime = 0.0f;

float t = 0; // The parameter for the Bezier curve, from 0 to 1
int p0[] = { 200, 475 }; // Control Point 0
int p1[] = { 300, 325 }; // Control Point 1
int p2[] = { 500, 325 }; // Control Point 2
int p3[] = { 600, 275 }; // Control Point 3
int targetPos[2];

void main(int argc, char** argr) {
	glutInit(&argc, argr);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Assignment 1");
	srand(time(NULL));
	initBackground();
	glutDisplayFunc(Display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(passiveMotion);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gluOrtho2D(0.0, 800.0, 0.0, 600.0);
	glutTimerFunc(0, timer, 0);
	glutMainLoop();
}

void initBackground() {
	backgroundStars.clear();
	for (int i = 0; i < 100; i++) {
		Point star;
		star.x = rand() % 800;
		star.y = rand() % 500 + 100;
		backgroundStars.push_back(star);
	}
}

// Draws the stars on the screen
void drawBackground() {
	glPointSize(2.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
	for (const auto& star : backgroundStars) {
		glVertex2f(star.x, star.y);
	}
	glEnd();
	glPointSize(1.0f);
}

void gameClockTimer(int value) {
	if (currentState == PLAYING) {

		if (isSpeedBoostActive) {
			speedBoostDuration--;
			if (speedBoostDuration <= 0) {
				isSpeedBoostActive = false;
				printf("Speed Boost Wore Off.\n");
			}
		}

		if (isShieldActive) {
			shieldDuration--;
			if (shieldDuration <= 0) {
				isShieldActive = false;
				printf("Shield Wore Off.\n");
			}
		}

		if (gameTime > 0) {
			gameTime--;
		}
		else {
			printf("GAME OVER - Time ran out!\n");
			currentState = GAME_OVER_LOSS;
		}

		glutPostRedisplay();
		glutTimerFunc(1000, gameClockTimer, 0);
	}
}

void mouse(int button, int state, int x, int y) {
	if (currentState != SETUP) {
		return;
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {


		int invertedY = 600 - y;
		printf("Mouse clicked at: x=%d, y=%d\n", x, invertedY);

		
		if (invertedY > 0 && invertedY < 100) {
			if (x > 140 && x < 160) {
				drawingMode = 1;
				printf("Drawing mode activated: OBSTACLE\n");
			}
			else if (x > 290 && x < 310) {
				drawingMode = 2;
				printf("Drawing mode activated: COLLECTIBLE\n");
			}
			else if (x > 440 && x < 460) {
				drawingMode = 3;
				printf("Drawing mode activated: POWER-UP 1\n");
			}
			else if (x > 590 && x < 610) {
				drawingMode = 4;
				printf("Drawing mode activated: POWER-UP 2\n");
			}
		}

		else if (invertedY > 100 && invertedY < 500) {
			Point newPoint = { (float)x, (float)invertedY };
			switch (drawingMode) {
			case 1:
				obstacles.push_back(newPoint);
				break;
			case 2:
				collectibles.push_back(newPoint);
				break;
			case 3:
				powerups1.push_back(newPoint);
				break;
			case 4:
				powerups2.push_back(newPoint);
				break;
			}
		}
		else if (invertedY > 100 && invertedY < 500) {
			Point newPoint = { (float)x, (float)invertedY };
			switch (drawingMode) {
				
			}
			drawingMode = 0;
		}
	}
}

void passiveMotion(int x, int y) {
	cursorX = x;
	cursorY = 600 - y;
	glutPostRedisplay();
}

bool checkCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
	float distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
	if (distance < r1 + r2) {
		return true;
	}
	return false;
}

void bezier(float t, int* p0, int* p1, int* p2, int* p3, int* result) {
	result[0] = pow((1 - t), 3) * p0[0] + 3 * t * pow((1 - t), 2) * p1[0] + 3 * pow(t, 2) * (1 - t) * p2[0] + pow(t, 3) * p3[0];
	result[1] = pow((1 - t), 3) * p0[1] + 3 * t * pow((1 - t), 2) * p1[1] + 3 * pow(t, 2) * (1 - t) * p2[1] + pow(t, 3) * p3[1];
}

void drawTarget() {
	glPushMatrix();
	glTranslatef(targetPos[0], targetPos[1], 0);

	
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(0, 0);
	for (int i = 0; i <= 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(20 * cos(angle), 20 * sin(angle));
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0, 0);
	for (int i = 0; i <= 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(10 * cos(angle), 10 * sin(angle));
	}
	glEnd();

	glPopMatrix();
}

void timer(int value) {
	animationTime += 0.05f;

	for (auto& star : backgroundStars) {
		star.y -= 0.5f;
		if (star.y < 100) {
			star.y = 600;
			star.x = rand() % 800;
		}
	}

	if (isTargetMovingForward) {
		t += 0.005;
		if (t >= 1.0) {
			t = 1.0;
			isTargetMovingForward = false;
		}
	}
	else {
		t -= 0.005;
		if (t <= 0.0) {
			t = 0.0;
			isTargetMovingForward = true;
		}
	}
	bezier(t, p0, p1, p2, p3, targetPos);
	if (currentState == PLAYING) {
		
		for (int i = collectibles.size() - 1; i >= 0; i--) {
			if (checkCollision(playerX, playerY, PLAYER_RADIUS, collectibles[i].x, collectibles[i].y, COLLECTIBLE_RADIUS)) {
				score += 5;
				collectibles.erase(collectibles.begin() + i);
				printf("Collectible picked up! New score: %d\n", score);
			}
		}
		for (int i = powerups1.size() - 1; i >= 0; i--) {
			if (checkCollision(playerX, playerY, PLAYER_RADIUS, powerups1[i].x, powerups1[i].y, POWERUP_RADIUS)) {
				isSpeedBoostActive = true;
				speedBoostDuration = 5;
				powerups1.erase(powerups1.begin() + i);
				printf("Speed Boost Activated!\n");
			}
		}

		for (int i = powerups2.size() - 1; i >= 0; i--) {
			if (checkCollision(playerX, playerY, PLAYER_RADIUS, powerups2[i].x, powerups2[i].y, POWERUP_RADIUS)) {
				isShieldActive = true;
				shieldDuration = 5;
				powerups2.erase(powerups2.begin() + i);
				printf("Shield Activated!\n");
			}
		}

		if (checkCollision(playerX, playerY, PLAYER_RADIUS, targetPos[0], targetPos[1], TARGET_RADIUS)) {
			currentState = GAME_OVER_WIN;
			printf("YOU WIN! Touched the target!\n");
		}
	}

	glutPostRedisplay();
	glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
	
	if ((key == 'r' || key == 'R') && currentState == SETUP) {
		currentState = PLAYING;
		printf("Game Started!\n");
		glutTimerFunc(1000, gameClockTimer, 0);
	}
	if (key == 13 && (currentState == GAME_OVER_WIN || currentState == GAME_OVER_LOSS)) {
		resetGame();
	}

	
	if (currentState == PLAYING) {
		
		float prevX = playerX;
		float prevY = playerY;

		
		int playerSpeed = isSpeedBoostActive ? 20 : 10;

		
		if (key == 'w' || key == 'W') {
			if ((playerY + playerSpeed) + 25 > 500) {
				playerY = 500 - 25;
				PlaySound(TEXT("punch-sound.wav"), NULL, SND_FILENAME | SND_ASYNC);
			}
			else {
				playerY += playerSpeed;
			}
			playerRotation = 0.0f;
		}

		
		if (key == 's' || key == 'S') {
			if ((playerY - playerSpeed) - 25 < 100) {
				playerY = 100 + 25;
				PlaySound(TEXT("punch-sound.wav"), NULL, SND_FILENAME | SND_ASYNC);
			}
			else {
				playerY -= playerSpeed;
			}
			playerRotation = 180.0f;
		}
		
		if (key == 'd' || key == 'D') {
			if ((playerX + playerSpeed) + 25 > 800) {
				playerX = 800 - 25;
				PlaySound(TEXT("punch-sound.wav"), NULL, SND_FILENAME | SND_ASYNC);
			}
			else {
				playerX += playerSpeed;
			}
			playerRotation = -90.0f;
		}

		
		if (key == 'a' || key == 'A') {
			if ((playerX - playerSpeed) - 25 < 0) {
				playerX = 0 + 25;
				PlaySound(TEXT("punch-sound.wav"), NULL, SND_FILENAME | SND_ASYNC);
			}
			else {
				playerX -= playerSpeed;
			}
			playerRotation = 90.0f;
		}

		if (!isShieldActive) {
			for (const auto& obs : obstacles) {
				if (checkCollision(playerX, playerY, PLAYER_RADIUS, obs.x, obs.y, OBSTACLE_RADIUS)) {
					lives--;
					playerX = prevX;
					playerY = prevY;
					printf("Hit an obstacle! Lives remaining: %d\n", lives);
					break;
				}
			}
		}

		
		if (lives <= 0) {
			currentState = GAME_OVER_LOSS;
		}
	}

	glutPostRedisplay();
}



void drawPlayer() {
	glPushMatrix();
	glTranslatef(playerX, playerY, 0);


	if (isShieldActive) {
		
		float pulse = 0.5f * (1.0f + sin(animationTime * 4.0f));
		float shieldRadius = PLAYER_RADIUS + 8.0f + (pulse * 4.0f);

		glLineWidth(3.0);
		
		glColor4f(0.0f, 0.8f, 1.0f, 0.6f);

		glBegin(GL_LINE_LOOP);
		for (int i = 0; i <= 360; i += 15) {
			float angle = i * 3.14159 / 180;
			glVertex2f(shieldRadius * cos(angle), shieldRadius * sin(angle));
		}
		glEnd();
		glLineWidth(1.0);
	}
	
	glRotatef(playerRotation, 0, 0, 1);

	
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(1.0f, 0.8f, 0.6f);
	glVertex2f(0, 15);
	for (int i = 0; i <= 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(10 * cos(angle), 15 + 10 * sin(angle));
	}
	glEnd();

	glBegin(GL_QUADS);
	glColor3f(0.2f, 0.5f, 1.0f);
	glVertex2f(-5, 5);
	glVertex2f(5, 5);
	glVertex2f(5, -10);
	glVertex2f(-5, -10);
	glEnd();


	glLineWidth(5.0);
	glBegin(GL_LINE_STRIP);
	glColor3f(1.0f, 0.8f, 0.6f);
	glVertex2f(-15, 0);
	glVertex2f(0, 0);
	glVertex2f(15, 0);
	glEnd();
	glLineWidth(1.0);

	glLineWidth(5.0);
	glBegin(GL_LINES);
	glColor3f(0.1f, 0.1f, 0.4f);
	
	glVertex2f(0, -10);
	glVertex2f(-10, -25);
	
	glVertex2f(0, -10);
	glVertex2f(10, -25);
	glEnd();
	glLineWidth(1.0);

	glPopMatrix();
}

void drawObstacle(float x, float y) {
	glPushMatrix();
	glTranslatef(x, y, 0);

	
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.2f, 0.2f, 0.2f);
	glVertex2f(0, 0);
	for (int i = 0; i <= 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(12 * cos(angle), 12 * sin(angle));
	}
	glEnd();


	glLineWidth(3.0);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	for (int i = 0; i < 360; i += 45) {
		float angle = i * 3.14159 / 180;
		glVertex2f(0, 0);
		glVertex2f(20 * cos(angle), 20 * sin(angle));
	}
	glEnd();
	glLineWidth(1.0);

	glPopMatrix();
}

void drawCollectible(float x, float y) {

	float yOffset = 5.0f * sin(animationTime);
	glPushMatrix();
	glTranslatef(x, y + yOffset, 0);

	
	glBegin(GL_POLYGON);
	glColor3f(1.0f, 0.8f, 0.0f);
	for (int i = 0; i < 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(20 * cos(angle), 20 * sin(angle));
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.8f, 0.6f, 0.0f);
	glVertex2f(0, 0);
	for (int i = 0; i <= 360; i++) {
		float angle = i * 3.14159 / 180;
		glVertex2f(10 * cos(angle), 10 * sin(angle));
	}
	glEnd();

	glPointSize(5.0);
	glBegin(GL_POINTS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0, 0);
	glEnd();
	glPointSize(1.0);

	glPopMatrix();
}

void drawPowerUp1(float x, float y) {
	glPushMatrix();
	glTranslatef(x, y, 0);
	float scale = 1.0f + 0.2f * sin(animationTime * 2.0f);
	glScalef(scale, scale, 1.0f);
	
	glBegin(GL_QUADS);
	glColor3f(0.2f, 0.8f, 1.0f);
	glVertex2f(-15, -5);
	glVertex2f(0, -5);
	glVertex2f(0, 5);
	glVertex2f(-15, 5);

	glVertex2f(0, -10);
	glVertex2f(15, -10);
	glVertex2f(15, 10);
	glVertex2f(0, 10);
	glEnd();

	
	glBegin(GL_TRIANGLES);
	glVertex2f(15, 20);
	glVertex2f(15, -20);
	glVertex2f(30, 0);
	glEnd();

	glPopMatrix();
}


void drawPowerUp2(float x, float y) {
	glPushMatrix();
	glTranslatef(x, y, 0);
	float scale = 1.0f + 0.2f * sin(animationTime * 2.0f);
	glScalef(scale, scale, 1.0f);
	
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-20, 20);
	glVertex2f(-15, -25);
	glVertex2f(0, 25);
	glVertex2f(0, -20);
	glVertex2f(20, 20);
	glVertex2f(15, -25);
	glEnd();

	
	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-20, 20);
	glVertex2f(0, 25);
	glVertex2f(20, 20);
	glVertex2f(15, -25);
	glVertex2f(-15, -25);
	glEnd();

	glPopMatrix();
}

void resetGame() {
	currentState = SETUP;
	gameTime = 90;
	score = 0;
	lives = 5;
	playerX = 25;
	playerY = 125;
	isSpeedBoostActive = false;
	isShieldActive = false;
	speedBoostDuration = 0;
	shieldDuration = 0;

	obstacles.clear();
	collectibles.clear();
	powerups1.clear();
	powerups2.clear();
	initBackground();
	printf("Game has been reset to setup mode.\n");
}

void print(int x, int y, char* string)
{
	int len, i;
	glRasterPos2f(x, y);
	len = (int)strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
	}
}

void Display() {
	glClear(GL_COLOR_BUFFER_BIT);

	drawBackground();

	glBegin(GL_POLYGON);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(800.0f, 0.0f, 0.0f);
	glVertex3f(800.0f, 100.0f, 0.0f);
	glVertex3f(0.0f, 100.0f, 0.0f);
	glEnd();

	glBegin(GL_POLYGON);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 500.0f, 0.0f);
	glVertex3f(800.0f, 500.0f, 0.0f);
	glVertex3f(800.0f, 600.0f, 0.0f);
	glVertex3f(0.0f, 600.0f, 0.0f);
	glEnd();
	if (currentState == SETUP) {
		glColor3f(1.0f, 1.0f, 1.0f);
		print(220, 300, "Setup the game, then press 'R' to start!");
	}
	if (currentState == SETUP && drawingMode != 0) {
		switch (drawingMode) {
		case 1: drawObstacle(cursorX, cursorY); break;
		case 2: drawCollectible(cursorX, cursorY); break;
		case 3: drawPowerUp1(cursorX, cursorY); break;
		case 4: drawPowerUp2(cursorX, cursorY); break;
		}
	}
	else if (currentState == GAME_OVER_LOSS) {
		glColor3f(1.0f, 0.0f, 0.0f);
		print(320, 320, "YOU LOST!");

		char finalScore[50];
		sprintf(finalScore, "Final Score: %d", score);
		print(310, 280, finalScore);

		glColor3f(1.0f, 1.0f, 1.0f);
		print(280, 240, "Press Enter to Restart");
	}
	else if (currentState == GAME_OVER_WIN) {
		glColor3f(0.0f, 1.0f, 0.0f);
		print(320, 320, "YOU WIN!");

		char finalScore[50];
		sprintf(finalScore, "Final Score: %d", score);
		print(310, 280, finalScore);

		glColor3f(1.0f, 1.0f, 1.0f);
		print(280, 240, "Press Enter to Restart");
	}

	
	glColor3f(1, 1, 1);
	char scoreString[20];
	sprintf(scoreString, "Score: %d", score);
	print(50, 550, scoreString);

	
	char timeString[20];
	sprintf(timeString, "Time: %d", gameTime);
	print(350, 550, timeString);

	
	for (int i = 0; i < lives; i++) {
		
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glVertex2f(650 + (i * 30), 540);
		glVertex2f(670 + (i * 30), 540);
		glVertex2f(670 + (i * 30), 560);
		glVertex2f(650 + (i * 30), 560);
		glEnd();

		
		glColor3f(1, 0, 0);
		glBegin(GL_QUADS);
		glVertex2f(652 + (i * 30), 542);
		glVertex2f(668 + (i * 30), 542);
		glVertex2f(668 + (i * 30), 558);
		glVertex2f(652 + (i * 30), 558);
		glEnd();
	}

	
	drawObstacle(150, 50);
	drawCollectible(300, 50);
	drawPowerUp1(450, 50);
	drawPowerUp2(600, 50);

	
	for (const auto& obs : obstacles) {
		drawObstacle(obs.x, obs.y);
	}
	for (const auto& col : collectibles) {
		drawCollectible(col.x, col.y);
	}
	for (const auto& p1 : powerups1) {
		drawPowerUp1(p1.x, p1.y);
	}
	for (const auto& p2 : powerups2) {
		drawPowerUp2(p2.x, p2.y);
	}


	if (currentState == PLAYING) {
		drawPlayer();
		drawTarget();
	}

	glutSwapBuffers();
}
