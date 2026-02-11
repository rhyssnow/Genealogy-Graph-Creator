#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <unordered_set>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <cmath>

using namespace rapidjson;
using namespace boost;

std::vector <std::pair<std::string, std::string>> people;
std::vector <std::pair<std::pair<std::string, std::string>,std::pair<float, float>>> connections;
Uint64 fakeFPS = 1000;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph; // Define the graph type
Graph g;


struct vertice {
    SDL_Surface *nameSurface;
    SDL_Texture *nameTexture;
    SDL_FRect nameArea;

    float divider = 0;
    float radius;
    int me;
    Uint8 red = 255;

    float x,y;
    float modX,modY;
    std::string nameToRender;

    vertice(std::string name, SDL_Renderer *renderer, int moi, TTF_Font * font) {
        nameToRender = name;
        me = moi;

        SDL_Color fontColor = {255, 255, 255, SDL_ALPHA_OPAQUE};
        nameSurface = TTF_RenderText_Blended(font, nameToRender.c_str(), 0, fontColor);
        if (nameSurface) {
            nameTexture = SDL_CreateTextureFromSurface(renderer, nameSurface);
        }
        SDL_DestroySurface(nameSurface);

        std::unordered_set<int> visited_set;
        std::vector<int> visited_vec;

        visited_set.insert(me);
        visited_vec.push_back(me);

        auto [ei, ei_end] = out_edges(me, g);
        for (; ei != ei_end; ++ei) {
            int v = static_cast<int>(target(*ei, g));

            if (visited_set.insert(v).second) {
                visited_vec.push_back(v);
            }
        }

        radius = static_cast<float>(visited_vec.size()) / 2;
        if (radius > static_cast<float>(people.size())/5) {
            radius = static_cast<float>(people.size())/5;
        }
        if (radius < 1) {
            radius = 1;
        }

        x = static_cast<float>(random() % 600);
        y = static_cast<float>(random() % 600);
    }

    ~vertice() {
        SDL_DestroyTexture(nameTexture);
    }

    void input(const bool* key) {
        divider = static_cast<float>(fakeFPS);
        if (divider == 0) {
            divider = 1;
        }
        if (key[SDL_SCANCODE_UP] == true) {
            y+=0.032*static_cast<float>(people.size());
        }
        if (key[SDL_SCANCODE_DOWN] == true) {
            y-=0.032*static_cast<float>(people.size());
        }
        if (key[SDL_SCANCODE_LEFT] == true) {
            x+=0.032*static_cast<float>(people.size());
        }
        if (key[SDL_SCANCODE_RIGHT] == true) {
            x-=0.032*static_cast<float>(people.size());
        }
        moveVertice();
    }

    void update(float changeX, float changeY) {
        x += changeX;
        y += changeY;
    }

    void moveVertice() {
        float nameW = static_cast<float>(nameToRender.size()) * radius/2+2;
        nameArea = SDL_FRect {
            .x=x-nameW/2,
            .y=y-radius*2-5,
            .w=nameW,
            .h=radius
        };
    }

    void render(SDL_Renderer *renderer) {
        moveVertice();
        filledCircleRGBA(renderer, x, y, radius, red, 255, 255, 255);
        SDL_RenderTexture(renderer, nameTexture, nullptr, &nameArea);
    }
};

struct SDLApplication {
    SDL_Window *mWindow;
    SDL_Renderer *mRenderer;
    std::map <std::string, int> peopleToNumbers;
    std::map <std::string, float> peopleRank;
    std::vector <vertice*> peopleAsVertices;
    bool mFullscreen = false;
    bool pausedGraphMovement = false;
    bool mRunning = true;
    TTF_Font *font;

    //constructor
    SDLApplication(const char* title) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        }
        if (!TTF_Init()) {
            SDL_Log("not able to init ttf fonts");
        }
        font = TTF_OpenFont("Assets/DovesTypeText-Regular.ttf", 100);

        if (!SDL_CreateWindowAndRenderer(title, 1920, 1080, SDL_WINDOW_RESIZABLE, &mWindow, &mRenderer)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        } else {
            SDL_Log("Renderer: %s=s", SDL_GetRendererName(mRenderer));
        }

        SDL_SetRenderLogicalPresentation(
            mRenderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_LETTERBOX
        );

        /* build graph + map */
        for (int i = 0; i < people.size(); i++) {
            boost::add_vertex(g);
            peopleToNumbers.insert({ people.at(i).second, i });
        }

        for (auto& i : connections) {
            boost::add_edge(
                peopleToNumbers[i.first.first],
                peopleToNumbers[i.first.second],
                g
            );
        }

        for (int i = 0; i < people.size(); i++) {
            initializeVerticesClass(i);
        }

        peopleAsVertices.at(0)->x = 300;
        peopleAsVertices.at(0)->y = 300;
        peopleAsVertices.at(0)->red = 0;

        for (int i = static_cast<int>(people.size()) - 1; i >= 0; i--) {

            std::unordered_set<int> visited_set;
            std::vector<int> visited_vec;

            visited_set.insert(i);
            visited_vec.push_back(i);

            auto [ei, ei_end] = out_edges(i, g);
            for (; ei != ei_end; ++ei) {
                int v = static_cast<int>(target(*ei, g));
                if (visited_set.insert(v).second) {
                    visited_vec.push_back(v);
                }
            }

            peopleRank.insert({ people.at(i).second, static_cast<float>(visited_vec.size())});
        }
    }


    void initializeVerticesClass (int pos) {
        peopleAsVertices.emplace_back(new vertice (people.at(pos).first, mRenderer, pos, font));
    }

    //destructor
    ~SDLApplication() {
        SDL_Quit();
    }

    //handle input events devices
    int w = 1920;
    int h = 1080;

    void input() {
        SDL_Event mEvent;
        const bool* keys = SDL_GetKeyboardState(nullptr);

        for (auto i : peopleAsVertices) {
            i->input(keys);
        }


        while (SDL_PollEvent(&mEvent)) {
            if (mEvent.type == SDL_EVENT_QUIT) {
                mRunning = false;
            }
            else if (mEvent.type == SDL_EVENT_KEY_DOWN) {
                if (mEvent.key.key == SDLK_F11) {
                    mFullscreen = !mFullscreen;
                    SDL_SetWindowFullscreen(mWindow, mFullscreen);
                }
                else if (mEvent.key.key == SDLK_SPACE) {
                    pausedGraphMovement = !pausedGraphMovement;
                }
            }
            else if (mEvent.type == SDL_EVENT_MOUSE_WHEEL) {
                h -= static_cast<int>(mEvent.wheel.y) * 50;
                w = h*1.77777777777777;
                SDL_SetRenderLogicalPresentation(mRenderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            }
        }
    }

    //updates logic
    void updateDistance(int pointA, int pointB, float strength, float weightPower, bool aBigger) {
        float largestConnection = 7000;

        auto pos1 = peopleAsVertices.at(pointA);
        auto pos2 = peopleAsVertices.at(pointB);

        float dx = pos2->x - pos1->x;
        float dy = pos2->y - pos1->y;

        float distance = sqrtf(dx*dx + dy*dy);

        if (distance == 0) return;

        float weight = largestConnection / weightPower;

        float delta = distance - strength;
        float force = delta / weight;

        float mx = dx /distance * force;
        float my = dy /distance * force;

        if (aBigger) {
            pos2->update(-mx, -my);
        }
        else {
            pos1->update( mx, my);
        }
    }

    void updateConnections() {
        if (!pausedGraphMovement) return;
        for (auto&[fst, snd] : connections) {
            if (peopleRank.at(fst.first) > peopleRank.at(fst.second)) {
                updateDistance(peopleToNumbers[fst.first], peopleToNumbers[fst.second], snd.first, peopleRank.at(fst.first)+peopleRank.at(fst.second)+snd.second, true);
            }
            else {
                updateDistance(peopleToNumbers[fst.first], peopleToNumbers[fst.second], snd.first, peopleRank.at(fst.first)+peopleRank.at(fst.second)+snd.second, false);
            }
        }
    }
    //renders window
    void render() {
        SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(mRenderer);

        SDL_SetRenderDrawColor(mRenderer, 40, 40, 40, 0xFF);

        auto y = connections;
        auto h = people;

        for (int i = 0; i < connections.size(); i++) {
            SDL_RenderLine(mRenderer, peopleAsVertices.at(peopleToNumbers.at(connections.at(i).first.first))->x, peopleAsVertices.at(peopleToNumbers.at(connections.at(i).first.first))->y, peopleAsVertices.at(peopleToNumbers.at(connections.at(i).first.second))->x, peopleAsVertices.at(peopleToNumbers.at(connections.at(i).first.second))->y);
        }

        for (auto i : peopleAsVertices) {
            i->render(mRenderer);
        }

        SDL_RenderPresent(mRenderer);
    }

    //main application loop
    void mainLoop() {
        Uint64 fps = 0;
        Uint64 lastTime = 0;
        while (mRunning) {
            Uint64 currentTick = SDL_GetTicks();

            advanceFrame();
            fps++;

            if (currentTick > lastTime + 5000) {
                lastTime = currentTick;
                std::string title;
                title += "Genealogy Graph Prototype - FPS: " + std::to_string(fps/5);
                SDL_SetWindowTitle(mWindow, title.c_str());
                fps = 0;
                if (fakeFPS == 1000000) {
                    fakeFPS = fps;
                }
            }
        }
    }

    void advanceFrame() {
        updateConnections();
        input();
        render();
    }
};

void JSONToVector() {
    std::cout << "What file would you like to access? ";
    std::string filepath = "Assets/";
    std::string file;
    std::cin >> file;
    file += ".json";
    filepath += file;
    FILE* fp = fopen(filepath.c_str(), "rb");

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    if (doc.HasMember("nodes") && doc["nodes"].IsArray()) {
        const rapidjson::Value& nodes = doc["nodes"];
        for (rapidjson::SizeType i = 0; i < nodes.Size(); i++) {
            std::pair <std::string, std::string> node = {nodes[i]["label"].GetString(), nodes[i]["id"].GetString()};
            people.push_back(node);
        }
    }
    if (doc.HasMember("links") && doc["links"].IsArray()) {
        const rapidjson::Value& links = doc["links"];
        for (rapidjson::SizeType i = 0; i < links.Size(); i++) {
            std::pair <std::pair<std::string, std::string>,std::pair <float, float>> node = {{links[i]["source"].GetString(), links[i]["target"].GetString()}, {static_cast<float>(people.size())*85/links[i]["strength"].GetFloat(), links[i]["strength"].GetFloat()}};
            connections.push_back(node);
        }
    }
}

int main(int argc, char *argv[]) {
    JSONToVector();
    SDLApplication app("Genealogy Graph Prototype");
    app.mainLoop();
    return 0;
}
