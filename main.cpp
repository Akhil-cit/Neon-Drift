#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <cmath>

using namespace sf;

enum State { MAIN_MENU, SETTINGS_HUB, DIFFICULTY_MENU, SOUND_MENU, CONTROLS_MENU, COUNTDOWN, START_VOICE, PLAYING, PAUSED, GAMEOVER };

struct Particle {
    CircleShape shape;
    float lifetime;
    float speed;
    float horizontalVel; 
};

struct SpeedLine {
    RectangleShape line;
    float speed;
};

void centerText(Text& text, float y, float windowWidth = 800.0f) {
    FloatRect bounds = text.getGlobalBounds();
    text.setPosition((windowWidth - bounds.width) / 2.0f, y);
}

std::string keyToString(Keyboard::Key key) {
    if (key >= 0 && key <= 25) return std::string(1, (char)(key + 65));
    if (key >= 26 && key <= 35) return std::to_string(key - 26);
    if (key == Keyboard::LShift) return "LShift";
    if (key == Keyboard::Space) return "Space";
    if (key == Keyboard::Left) return "Left";
    if (key == Keyboard::Right) return "Right";
    if (key == Keyboard::Enter) return "Enter";
    if (key == Keyboard::Escape) return "Esc";
    if (key == Keyboard::Num1 || key == Keyboard::Numpad1) return "1";
    if (key == Keyboard::Num2 || key == Keyboard::Numpad2) return "2";
    return "K:" + std::to_string(key);
}

int main() {
    RenderWindow window(VideoMode(800, 1000), "NEON DRIFT", Style::Close);
    window.setFramerateLimit(60);

    Font font; font.loadFromFile("meme.ttf");
    Texture tRoad, tCar, tObs, tCoin;
    tRoad.loadFromFile("road.png"); tCar.loadFromFile("car.png");
    tObs.loadFromFile("obstacle.png"); tCoin.loadFromFile("coin.png");

    tRoad.setRepeated(true);
    Sprite sRoad(tRoad);
    float scaleX = 800.0f / tRoad.getSize().x;
    sRoad.setScale(scaleX, 1.0f); 
    IntRect rectSource(0, 0, tRoad.getSize().x, 1000);
    sRoad.setTextureRect(rectSource);

    Sprite uiCoinIcon(tCoin); uiCoinIcon.setScale(1.2f, 1.2f); uiCoinIcon.setPosition(5, 15);

    SoundBuffer bCoin, bCrash, bStart;
    bCoin.loadFromFile("coinss.wav"); bCrash.loadFromFile("crash.wav"); bStart.loadFromFile("aarambikalam.wav");
    Sound sCoin(bCoin), sCrash(bCrash), sStart(bStart);
    Music radio;

    Keyboard::Key keyLeft = Keyboard::Left, keyRight = Keyboard::Right;
    Keyboard::Key keyNitro = Keyboard::LShift, keyPause = Keyboard::Space;
    Keyboard::Key keyRad1 = Keyboard::Num1, keyRad2 = Keyboard::Num2;

    State gameState = MAIN_MENU;
    int menuCursor = 0, hubCursor = 0, controlCursor = 0, diffCursor = 1, diffSelection = 1, pauseCursor = 0; 
    std::string diffNames[] = {"EASY", "MEDIUM", "HARD"};
    float diffMultipliers[] = {0.7f, 1.0f, 1.4f};

    int score = 0, currentStation = 1;
    float speed = 450.f, scrollY = 0, spawnTimer = 0, countdownTimer = 3.9f;
    float globalVolume = 50.f, gameTimer = 0.f, nitroTimer = 0.f; 
    float shakeTimer = 0.f, splashAlpha = 0.f; 
    bool isNitroActive = false, isWaitingForKey = false;

    RectangleShape nitroBarFill(Vector2f(200, 15));
    nitroBarFill.setFillColor(Color::Red); nitroBarFill.setPosition(300, 85);
    RectangleShape nitroBarBg(Vector2f(200, 15));
    nitroBarBg.setFillColor(Color(50,50,50)); nitroBarBg.setPosition(300, 85);

    Sprite car(tCar); car.setOrigin(tCar.getSize().x / 2.f, tCar.getSize().y / 2.f);
    car.setScale(2.0f, 2.0f); 

    std::vector<Sprite> obstacles, coins;
    std::vector<Particle> particles;
    std::vector<SpeedLine> nitroLines;
    Clock frameClock;

    auto playStation = [&](int station) {
        radio.stop(); currentStation = station;
        if (radio.openFromFile("radio" + std::to_string(station) + ".wav")) {
            radio.setVolume(globalVolume); radio.setLoop(true); radio.play();
        }
    };

    auto resetGame = [&]() {
        score = 0; speed = 450.f; scrollY = 0; gameTimer = 0; nitroTimer = 0; isNitroActive = false;
        shakeTimer = 0; splashAlpha = 0; obstacles.clear(); coins.clear(); particles.clear(); nitroLines.clear();
        car.setPosition(400, 850); car.setScale(2.0f, 2.0f); countdownTimer = 3.9f;
        window.setView(window.getDefaultView());
    };

    while (window.isOpen()) {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            if (e.type == Event::KeyPressed) {
                if (gameState == MAIN_MENU) {
                    if (e.key.code == Keyboard::Up) menuCursor = (menuCursor + 2) % 3;
                    if (e.key.code == Keyboard::Down) menuCursor = (menuCursor + 1) % 3;
                    if (e.key.code == Keyboard::Enter) {
                        if (menuCursor == 0) { resetGame(); gameState = COUNTDOWN; }
                        else if (menuCursor == 1) gameState = SETTINGS_HUB;
                        else window.close();
                    }
                }
                else if (gameState == SETTINGS_HUB) {
                    if (e.key.code == Keyboard::Up) hubCursor = (hubCursor + 2) % 3;
                    if (e.key.code == Keyboard::Down) hubCursor = (hubCursor + 1) % 3;
                    if (e.key.code == Keyboard::Enter) {
                        if (hubCursor == 0) gameState = DIFFICULTY_MENU;
                        else if (hubCursor == 1) gameState = SOUND_MENU;
                        else if (hubCursor == 2) gameState = CONTROLS_MENU;
                    }
                    if (e.key.code == Keyboard::Escape) gameState = MAIN_MENU;
                }
                else if (gameState == DIFFICULTY_MENU) {
                    if (e.key.code == Keyboard::Up) diffCursor = (diffCursor + 2) % 3;
                    if (e.key.code == Keyboard::Down) diffCursor = (diffCursor + 1) % 3;
                    if (e.key.code == Keyboard::Enter) { diffSelection = diffCursor; gameState = SETTINGS_HUB; }
                    if (e.key.code == Keyboard::Escape) gameState = SETTINGS_HUB;
                }
                else if (gameState == SOUND_MENU) {
                    if (e.key.code == Keyboard::Left && globalVolume > 0) globalVolume -= 10;
                    if (e.key.code == Keyboard::Right && globalVolume < 100) globalVolume += 10;
                    if (e.key.code == Keyboard::Escape) gameState = SETTINGS_HUB;
                }
                else if (gameState == CONTROLS_MENU) {
                    if (!isWaitingForKey) {
                        if (e.key.code == Keyboard::Up) controlCursor = (controlCursor + 5) % 6;
                        if (e.key.code == Keyboard::Down) controlCursor = (controlCursor + 1) % 6;
                        if (e.key.code == Keyboard::Enter) isWaitingForKey = true;
                        if (e.key.code == Keyboard::Escape) gameState = SETTINGS_HUB;
                    } else {
                        if (controlCursor == 0) keyLeft = e.key.code;
                        else if (controlCursor == 1) keyRight = e.key.code;
                        else if (controlCursor == 2) keyNitro = e.key.code;
                        else if (controlCursor == 3) keyPause = e.key.code;
                        else if (controlCursor == 4) keyRad1 = e.key.code;
                        else if (controlCursor == 5) keyRad2 = e.key.code;
                        isWaitingForKey = false;
                    }
                }
                else if (gameState == PLAYING) {
                    if (e.key.code == keyPause) { gameState = PAUSED; radio.pause(); }
                    else if (e.key.code == keyRad1) playStation(1);
                    else if (e.key.code == keyRad2) playStation(2);
                    else if (e.key.code == keyNitro && !isNitroActive && score >= 10) { score -= 10; nitroTimer = 5.0f; isNitroActive = true; }
                }
                else if (gameState == PAUSED) {
                    if (e.key.code == Keyboard::Up) pauseCursor = (pauseCursor + 2) % 3;
                    if (e.key.code == Keyboard::Down) pauseCursor = (pauseCursor + 1) % 3;
                    if (e.key.code == Keyboard::Enter) {
                        if (pauseCursor == 0) { gameState = PLAYING; radio.play(); }
                        else if (pauseCursor == 1) { resetGame(); gameState = COUNTDOWN; }
                        else { gameState = MAIN_MENU; radio.stop(); }
                    }
                }
                else if (gameState == GAMEOVER) {
                    if (e.key.code == Keyboard::R) { resetGame(); gameState = COUNTDOWN; }
                    if (e.key.code == Keyboard::M) { radio.stop(); gameState = MAIN_MENU; }
                }
            }
        }

        float dt = frameClock.restart().asSeconds();
        window.clear(Color(10, 10, 15));

        if (shakeTimer > 0) {
            shakeTimer -= dt;
            View view = window.getDefaultView();
            view.move((rand() % 25 - 12) * shakeTimer, (rand() % 25 - 12) * shakeTimer);
            window.setView(view);
        } else { window.setView(window.getDefaultView()); }

        float currentSpeed = (gameState == PLAYING) ? speed * diffMultipliers[diffSelection] : 150.0f;
        if (isNitroActive && gameState == PLAYING) currentSpeed *= 2.5f;
        
        scrollY -= currentSpeed * dt;
        rectSource.top = (int)scrollY;
        sRoad.setTextureRect(rectSource);
        window.draw(sRoad);

        if (gameState == PLAYING || gameState == PAUSED || gameState == GAMEOVER) {
            if (gameState == PLAYING) {
                gameTimer += dt; speed += 2.0f * dt;
                if (isNitroActive) {
                    nitroTimer -= dt; if (nitroTimer <= 0) isNitroActive = false;
                    nitroBarFill.setSize(Vector2f((nitroTimer / 5.0f) * 200.0f, 15));
                    if (rand() % 2 == 0) {
                        SpeedLine sl; sl.line.setSize(Vector2f(2, rand() % 50 + 30));
                        sl.line.setFillColor(Color(255, 255, 255, 100));
                        sl.line.setPosition(rand() % 800, -60); sl.speed = 2500.f;
                        nitroLines.push_back(sl);
                    }
                }
                for(int i=0; i<(isNitroActive ? 8 : 3); i++){
                    Particle p; p.shape.setRadius(rand() % 4 + 2);
                    p.shape.setFillColor(isNitroActive ? Color::Red : Color::Cyan);
                    p.shape.setPosition(car.getPosition().x + (rand() % 24 - 12), car.getPosition().y + 45);
                    p.lifetime = 1.0f; p.speed = currentSpeed * 0.4f;
                    p.horizontalVel = isNitroActive ? (float)(rand() % 400 - 200) : (float)(rand() % 40 - 20);
                    particles.push_back(p);
                }
                spawnTimer += dt;
                if (spawnTimer > std::max(0.4f, 1.2f - (speed / 3000.f))) {
                    int lane = (rand() % 5) * 110 + 180;
                    if (rand() % 10 < 4) {
                        int coinType = rand() % 3; 
                        int count = (coinType == 0) ? (rand() % 3 + 3) : (coinType == 1 ? 2 : 1);
                        for(int i=0; i<count; i++) {
                            Sprite c(tCoin); c.setOrigin(tCoin.getSize().x/2.f, tCoin.getSize().y/2.f);
                            c.setScale(2.5f, 2.5f); c.setPosition(lane, -100 - (i * 85)); coins.push_back(c);
                        }
                    } else { 
                        Sprite o(tObs); o.setOrigin(tObs.getSize().x/2.f, tObs.getSize().y/2.f);
                        o.setScale(1.7f, 1.7f); o.setPosition(lane, -100); obstacles.push_back(o); 
                    }
                    spawnTimer = 0;
                }
                float moveSpeed = isNitroActive ? 1400.f : 800.f;
                if (Keyboard::isKeyPressed(keyLeft) && car.getPosition().x > 140) car.move(-moveSpeed * dt, 0);
                if (Keyboard::isKeyPressed(keyRight) && car.getPosition().x < 660) car.move(moveSpeed * dt, 0);
            }

            for (auto& sl : nitroLines) { if (gameState == PLAYING) sl.line.move(0, sl.speed * dt); window.draw(sl.line); }
            for (auto it = particles.begin(); it != particles.end();) {
                if (gameState == PLAYING) { it->lifetime -= dt * 3.5f; it->shape.move(it->horizontalVel * dt, it->speed * dt); }
                Color clr = it->shape.getFillColor(); clr.a = (Uint8)(std::max(0.0f, it->lifetime * 255.0f));
                it->shape.setFillColor(clr); window.draw(it->shape);
                if (it->lifetime <= 0) it = particles.erase(it); else ++it;
            }
            for (auto it = coins.begin(); it != coins.end();) {
                if(gameState == PLAYING) it->move(0, currentSpeed*dt); window.draw(*it);
                if (gameState == PLAYING && car.getGlobalBounds().intersects(it->getGlobalBounds())) { score += 10; sCoin.play(); it = coins.erase(it); }
                else if (it->getPosition().y > 1100) it = coins.erase(it); else ++it;
            }
            for (auto it = obstacles.begin(); it != obstacles.end();) {
                if(gameState == PLAYING) it->move(0, currentSpeed*dt); window.draw(*it);
                if (gameState == PLAYING && car.getGlobalBounds().intersects(it->getGlobalBounds())) { 
                    gameState = GAMEOVER; sCrash.play(); radio.stop(); shakeTimer = 1.5f; splashAlpha = 255.f; 
                    isNitroActive = false; 
                }
                else if (it->getPosition().y > 1100) it = obstacles.erase(it); else ++it;
            }
            window.draw(car);
            if (splashAlpha > 0) {
                RectangleShape flash(Vector2f(800, 1000)); flash.setFillColor(Color(255, 255, 255, (Uint8)splashAlpha));
                window.draw(flash); splashAlpha -= dt * 400.f;
            }
            window.draw(uiCoinIcon);
            Text ts("SCORE: " + std::to_string(score), font, 30); ts.setPosition(50, 15); window.draw(ts);
            Text tt("TIMER: " + std::to_string((int)gameTimer) + "s", font, 30); tt.setPosition(600, 15); window.draw(tt);

            if (isNitroActive && gameState != GAMEOVER) {
                window.draw(nitroBarBg); window.draw(nitroBarFill);
                Text tn("NITRO", font, 20); tn.setPosition(375, 60); tn.setFillColor(Color::Red); window.draw(tn);
            }

            if (gameState == PAUSED) {
                RectangleShape dim(Vector2f(800, 1000)); dim.setFillColor(Color(0,0,0,180)); window.draw(dim);
                Text p("PAUSED", font, 80); centerText(p, 300); window.draw(p);
                std::string pOpts[] = {"RESUME", "RESTART", "MAIN MENU"};
                for(int i=0; i<3; i++) {
                    Text t(pOpts[i], font, 50); t.setFillColor(pauseCursor==i?Color::Yellow:Color::White); centerText(t, 500 + i*80); window.draw(t);
                }
            }
            if (gameState == GAMEOVER) { 
                Text g("CRASHED!", font, 90); g.setFillColor(Color::Red); centerText(g, 350); window.draw(g);
                Text gr("Press 'R' to RESTART", font, 40); gr.setFillColor(Color::White); centerText(gr, 500); window.draw(gr);
                Text gm("Press 'M' for MAIN MENU", font, 40); gm.setFillColor(Color::White); centerText(gm, 580); window.draw(gm);
            }
        }
        else if (gameState == MAIN_MENU) {
            Text t("NEON DRIFT", font, 100); centerText(t, 200); window.draw(t);
            std::string o[] = {"START", "SETTINGS", "EXIT"};
            for(int i=0; i<3; i++) { Text txt(o[i], font, 50); txt.setFillColor(menuCursor==i?Color::Yellow:Color::White); centerText(txt, 500+i*80); window.draw(txt); }
        }
        else if (gameState == SETTINGS_HUB) {
            Text t("SETTINGS", font, 80); centerText(t, 150); window.draw(t);
            std::string o[] = {"DIFFICULTY", "SOUND", "CONTROLS"};
            for(int i=0; i<3; i++) { Text txt(o[i], font, 50); txt.setFillColor(hubCursor==i?Color::Yellow:Color::White); centerText(txt, 400+i*80); window.draw(txt); }
        }
        else if (gameState == DIFFICULTY_MENU) {
            Text t("DIFFICULTY", font, 70); centerText(t, 200); window.draw(t);
            for(int i=0; i<3; i++) { Text txt(diffNames[i] + (diffSelection==i?" *":""), font, 50); txt.setFillColor(diffCursor==i?Color::Yellow:Color::White); centerText(txt, 400+i*80); window.draw(txt); }
        }
        else if (gameState == SOUND_MENU) {
            Text t("VOLUME: " + std::to_string((int)globalVolume), font, 70); centerText(t, 450); window.draw(t);
            Text h("LEFT/RIGHT TO CHANGE", font, 30); centerText(h, 550); window.draw(h);
        }
        else if (gameState == CONTROLS_MENU) {
            std::string l[] = {"Left", "Right", "Nitro", "Pause", "Radio 1", "Radio 2"};
            std::string k[] = {keyToString(keyLeft), keyToString(keyRight), keyToString(keyNitro), keyToString(keyPause), keyToString(keyRad1), keyToString(keyRad2)};
            for(int i=0; i<6; i++) { 
                Text txt(l[i] + ": " + (isWaitingForKey && controlCursor == i ? "?" : k[i]), font, 40); 
                txt.setFillColor(controlCursor == i ? Color::Cyan : Color::White); 
                centerText(txt, 250 + i * 70); window.draw(txt); 
            }
        }
        else if (gameState == COUNTDOWN) {
            countdownTimer -= dt; int count = (int)std::ceil(countdownTimer - 1.0f);
            if (count > 0) { Text c(std::to_string(count), font, 150); c.setFillColor(Color::Yellow); centerText(c, 400); window.draw(c); }
            if (countdownTimer <= 1.0f) { gameState = START_VOICE; sStart.play(); }
        }
        else if (gameState == START_VOICE) {
            Text v("Arambikalamaaa?", font, 80); v.setFillColor(Color::Cyan); centerText(v, 450); window.draw(v);
            if (sStart.getStatus() == Sound::Stopped) { gameState = PLAYING; playStation(1); }
        }
        window.display();
    }
    return 0;
}