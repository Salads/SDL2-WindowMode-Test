#include <SDL.h>
#include "imgui.h"
#include "imgui_sdl.h"
#include <vector>
#include <string>
using namespace std;

enum WindowMode
{
	Fullscreen = 0,
	BorderlessFullscreen = 1,
	Windowed = 2
};

struct Resolution
{
	int width;
	int height;
};

// Each display may have different video modes supported.
int currentDisplay = 0;
int numDisplays = 0;
int currentResIndex = 0;
WindowMode currentWindowMode;
const char* currentResStr = NULL;
vector<string> windowModeStrAr = {};
vector<string> displayIndexStrAr = {};
vector<vector<string>> resStrAr = {};
vector<vector<Resolution>> resAr = {};

void InitializeDisplayModes()
{
	displayIndexStrAr.clear();
	resStrAr.clear();
	resAr.clear();

	// Sneak in initializing window mode str vector
	windowModeStrAr.push_back("Fullscreen");
	windowModeStrAr.push_back("Borderless Fullscreen");
	windowModeStrAr.push_back("Windowed");

	numDisplays = SDL_GetNumVideoDisplays();
	for (int display = 0; display < numDisplays; ++display)
	{
		// Cache display index to string for performance.
		displayIndexStrAr.push_back(to_string(display));

		// Simply add vectors for each display index.
		resAr.push_back(vector<Resolution>());
		resStrAr.push_back(vector<string>());


		// Enumerate display modes for the specified display and 
		// save both string and actual resolution.
		int numModes = SDL_GetNumDisplayModes(display);
		for (int modeIndex = 0; modeIndex < numModes; ++modeIndex)
		{
			SDL_DisplayMode mode;
			SDL_GetDisplayMode(display, modeIndex, &mode);

			bool isNew = true;
			for (int x = 0; x < resAr[display].size(); ++x)
			{
				Resolution r = resAr[display][x];
				if (mode.h == r.height && mode.w == r.width)
				{
					isNew = false;
					break;
				}
			}

			if (isNew)
			{
				Resolution newRes = { mode.w, mode.h };
				resAr[display].push_back(newRes);

				resStrAr[display].push_back(to_string(newRes.width) + "x" + to_string(newRes.height));
			}
			
		}
	}
}

const char * GetWindowModeStr(WindowMode mode)
{
	if (mode == Fullscreen)
	{
		return "Fullscreen";
	}
	else if (mode == BorderlessFullscreen)
	{
		return "Fullscreen Windowed";
	}
	else if (mode == Windowed)
	{
		return "Windowed";
	}

	return "Unknown";
}

void ClearScreen(SDL_Window *window)
{
	SDL_Surface *screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));
	SDL_FreeSurface(screenSurface);

	SDL_UpdateWindowSurface(window);
}

int main(int argc, char* args[])
{
	//The window we'll be rendering to
	SDL_Window* window = NULL;

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		InitializeDisplayModes();

		Uint32 flags = SDL_WINDOW_SHOWN;
		window = SDL_CreateWindow("SDL Multi Monitor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, flags);
		currentWindowMode = Windowed;

		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get window surface
			screenSurface = SDL_GetWindowSurface(window);
			SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

			SDL_RendererInfo rendererInfo;
			SDL_GetRendererInfo(renderer, &rendererInfo);

			string title = SDL_GetWindowTitle(window);
			title += " [" + string(rendererInfo.name) + "]";
			SDL_SetWindowTitle(window, title.c_str());
			
			ImGui::CreateContext();
			ImGuiSDL::Initialize(renderer, 640, 480);
			ImGuiIO &io = ImGui::GetIO();

			int curW, curH = 0;
			SDL_GetWindowSize(window, &curW, &curH);
			for (int x = 0; x < resAr[currentDisplay].size(); ++x)
			{
				Resolution res = resAr[currentDisplay][x];
				if (res.height == curH && res.width == curW)
				{
					currentResIndex = x;
				}
			}

			bool quit = false;
			while (!quit)
			{
				SDL_Event e;
				while (SDL_PollEvent(&e) != 0)
				{
					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					else if (e.type == SDL_WINDOWEVENT)
					{
						if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
						{
							ImGuiIO& io = ImGui::GetIO();
							io.DisplaySize.x = static_cast<float>(e.window.data1);
							io.DisplaySize.y = static_cast<float>(e.window.data2);
						}
					}
				}

				// ImGui Controls and Render
				{
					// Pass input events to ImGui (ignoring some tho)
					int mousePosX;
					int mousePosY;
					Uint32 mouse_buttons = SDL_GetMouseState(&mousePosX, &mousePosY);
					io.MouseDown[0] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
					io.MouseDown[1] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
					io.MouseDown[2] = (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
					io.MousePos = ImVec2((float)mousePosX, (float)mousePosY);

					int posX, posY = 0;
					SDL_GetWindowPosition(window, &posX, &posY);

					int sizeX, sizeY = 0;
					SDL_GetWindowSize(window, &sizeX, &sizeY);

					int displayIndex = SDL_GetWindowDisplayIndex(window);

					SDL_Rect bounds;
					SDL_GetDisplayBounds(displayIndex, &bounds);

					Uint32 flags = SDL_GetWindowFlags(window);
					bool isRealFullscreen = ((flags & SDL_WINDOW_FULLSCREEN) != 0);
					bool isFakeFullscreen = ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0);
					bool isWindowed = (!isRealFullscreen && !isFakeFullscreen);

					SDL_version sdlCompileVersion;
					SDL_version sdlLinkedVersion;

					SDL_VERSION(&sdlCompileVersion);
					SDL_GetVersion(&sdlLinkedVersion);

					ImGui::NewFrame();
					ImGui::Begin("SDL2 Test", NULL, ImGuiWindowFlags_NoMove);
					ImGui::SetWindowPos(ImVec2(0, 0));
					ImGui::SetWindowSize(ImVec2((float)sizeX, (float)sizeY));
					{
						ImGui::BeginChild("Contents");
						{
							if (ImGui::BeginCombo("Display", displayIndexStrAr[currentDisplay].c_str(), ImGuiComboFlags_HeightRegular))
							{
								for (int x = 0; x < numDisplays; ++x)
								{
									if (ImGui::Selectable(displayIndexStrAr[x].c_str(), x == currentDisplay))
									{
										int oldDisplay = currentDisplay;
										currentDisplay = x;
										
										bool foundEqualRes = false;
										for (int i = 0; i < resAr[currentDisplay].size(); ++i)
										{
											Resolution r = resAr[currentDisplay][i];
											Resolution t = resAr[oldDisplay][currentResIndex];
											if (r.height == t.height && r.width == t.width)
											{
												currentResIndex = i;
												foundEqualRes = true;
												break;
											}
										}

										if (!foundEqualRes)
										{
											currentResIndex = 0;
										}
									}
								}

								ImGui::EndCombo();
							}

							if (ImGui::BeginCombo("Window Mode", windowModeStrAr[currentWindowMode].c_str()))
							{
								for (int x = 0; x < 3; ++x)
								{
									if (ImGui::Selectable(windowModeStrAr[x].c_str(), x == currentWindowMode))
									{
										currentWindowMode = (WindowMode)x;
									}
								}

								ImGui::EndCombo();
							}

							if (ImGui::BeginCombo("Resolution", resStrAr[currentDisplay][currentResIndex].c_str()))
							{
								for (int x = 0; x < resStrAr[currentDisplay].size(); ++x)
								{
									if (ImGui::Selectable(resStrAr[currentDisplay][x].c_str(), x == currentResIndex))
									{
										currentResIndex = x;
									}
								}

								ImGui::EndCombo();
							}

							if (ImGui::Button("Apply", ImVec2(150, 20)))
							{
								SDL_SetWindowFullscreen(window, 0);

								Resolution targetRes = resAr[currentDisplay][currentResIndex];
								SDL_SetWindowSize(window, targetRes.width, targetRes.height);

								SDL_Rect bounds;
								SDL_GetDisplayBounds(currentDisplay, &bounds);

								// Centered in hopes that the display will be correct but it's not. :(
								SDL_SetWindowPosition(window, bounds.x, bounds.y);

								if (currentWindowMode == Fullscreen)
								{
									SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
								}
								else if (currentWindowMode == BorderlessFullscreen)
								{
									SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
								}
								else if (currentWindowMode == Windowed)
								{
									SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(currentDisplay), SDL_WINDOWPOS_CENTERED_DISPLAY(currentDisplay));
								}
							}

							ImGui::Separator();
							ImGui::Columns(2);
							{
								// Column 1
								ImGui::Text("SDL2 Compile Version:");
								ImGui::Text("SDL2 Linked Version:");
								ImGui::Text("SDL2 Window Position:");
								ImGui::Text("SDL2 Window Size:");
								ImGui::Text("SDL2 Display Index:");
								ImGui::Text("SDL2 Display Bounds:");
								ImGui::Text("SDL_WINDOW_FULLSCREEN:");
								ImGui::Text("SDL_WINDOW_FULLSCREEN_DESKTOP:");
								ImGui::Text("SDL2 Is Windowed:");
								ImGui::Text("----------------------------");
								ImGui::Text("Abstract Window Mode:");
								ImGui::Text("GUI Display:");
								ImGui::Text("GUI Resolution Index:");
								ImGui::Text("GUI Resolution:");

								// Column 2
								ImGui::NextColumn();
								ImGui::Text("%d.%d.%d", sdlCompileVersion.major, sdlCompileVersion.minor, sdlCompileVersion.patch);
								ImGui::Text("%d.%d.%d", sdlLinkedVersion.major, sdlLinkedVersion.minor, sdlLinkedVersion.patch);
								ImGui::Text("( %d, %d )", posX, posY);
								ImGui::Text("%dx%d", sizeX, sizeY);
								ImGui::Text("%d", displayIndex);
								ImGui::Text("Position(%d,%d) Size(%d,%d)", bounds.x, bounds.y, bounds.w, bounds.h);
								ImGui::Text("%d", isRealFullscreen);
								ImGui::Text("%d", isFakeFullscreen);
								ImGui::Text("%d", isWindowed);
								ImGui::Text("----------------------------");
								ImGui::Text("%s", GetWindowModeStr(currentWindowMode));
								ImGui::Text("%d", currentDisplay);
								ImGui::Text("%d", currentResIndex);
								ImGui::Text("%s", resStrAr[currentDisplay][currentResIndex].c_str());
								
							}
						}

						ImGui::EndChild();
					}

					ImGui::End();
					ImGui::EndFrame();

					ImGui::Render();
					ImGuiSDL::Render(ImGui::GetDrawData());

					SDL_RenderPresent(renderer);
				}
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}