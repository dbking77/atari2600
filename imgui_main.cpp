// Dear ImGui: standalone example application for GLUT/FreeGLUT + OpenGL2, using legacy fixed pipeline

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// !!! GLUT/FreeGLUT IS OBSOLETE PREHISTORIC SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT today, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// On Windows, you can install Freeglut using vcpkg:
//   git clone https://github.com/Microsoft/vcpkg
//   cd vcpkg
//   bootstrap - vcpkg.bat0
//   vcpkg install freeglut --triplet=x86-windows   ; for win32
//   vcpkg install freeglut --triplet=x64-windows   ; for win64
//   vcpkg integrate install                        ; register include and libs in Visual Studio

#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"
#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <GL/glu.h>

#ifdef _MSC_VER
#pragma warning(disable : 4505) // unreferenced local function has been removed
#endif

#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

#include "atari2600.hpp"

// Forward declarations of helper functions
void MainLoopStep();

#include "misc/cpp/imgui_stdlib.h"

Atari2600 atari;

int main(int argc, char **argv)
{
  // Create GLUT window
  glutInit(&argc, argv);
#ifdef __FREEGLUT_EXT_H__
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
  glutInitWindowSize(1280, 720);
  glutCreateWindow("Dear ImGui GLUT+OpenGL2 Example");

  if (argc != 2)
  {
    std::cerr << "Specify ROM filename" << std::endl;
    return 1;
  }

  std::string rom_fn = argv[1];
  std::cout << "Loading ROM " << rom_fn << std::endl;
  // Load rom
  std::ifstream rom_input(rom_fn, std::ifstream::binary);
  if (!rom_input.good())
  {
    std::cerr << "ROM could not be openned" << std::endl;
    return 1;
  }
  atari.loadRom(rom_input);

  std::string palette_fn = "palette/REALNTSC.pal";
  std::cout << "Loading color palette " << palette_fn << std::endl;
  // Load rom
  std::ifstream palette_input(palette_fn, std::ifstream::binary);
  if (!palette_input.good())
  {
    std::cerr << "Palette could not be openned" << std::endl;
    return 1;
  }
  atari.tia_.loadPalette(palette_input);

  // Setup GLUT display function
  // We will also call ImGui_ImplGLUT_InstallFuncs() to get all the other functions installed for us,
  // otherwise it is possible to install our own functions and call the imgui_impl_glut.h functions ourselves.
  glutDisplayFunc(MainLoopStep);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  // FIXME: Consider reworking this example to install our own GLUT funcs + forward calls ImGui_ImplGLUT_XXX ones, instead of using ImGui_ImplGLUT_InstallFuncs().
  ImGui_ImplGLUT_Init();
  ImGui_ImplOpenGL2_Init();

  // Install GLUT handlers (glutReshapeFunc(), glutMotionFunc(), glutPassiveMotionFunc(), glutMouseFunc(), glutKeyboardFunc() etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
  ImGui_ImplGLUT_InstallFuncs();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // IM_ASSERT(font != nullptr);

  // Main loop
  glutMainLoop();

  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGLUT_Shutdown();
  ImGui::DestroyContext();

  return 0;
}

// Our state
static bool show_demo_window = true;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

class RamWindow
{
public:
  bool show_ = true;

  std::ostringstream ss_;

  void draw(Atari2600 &atari)
  {
    if (!show_)
    {
      return;
    }

    ImGui::Begin("RAM", &show_);
    const auto &ram = atari.ram_;
    for (unsigned idx = 0; idx < ram.size(); idx += 8)
    {
      const uint8_t *row = &ram[idx];
      const uint16_t addr = idx + 0x80; // RAM starts at 0x80
      ImGui::Text("%02X : %02X %02X %02X %02X  %02X %02X %02X %02X",
                  addr, row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7]);
    }
    ImGui::End();
  }
};

class Mos6502Window
{
public:
  bool show_ = true;

  std::ostringstream ss_;

  std::string toHexStr(unsigned value, unsigned width)
  {
    ss_.str("");
    ss_ << std::hex << std::setw(width) << std::setfill('0') << value;
    return ss_.str();
  }

  void draw(Atari2600 &atari)
  {
    Mos6502 &cpu = atari.cpu_;

    if (!show_)
    {
      return;
    }

    ImGui::Begin("6502", &show_); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

    if (ImGui::Button("Step"))
    {
      atari.execInstructions(1);
    }
    if (ImGui::Button("Step 10"))
    {
      atari.execInstructions(10);
    }
    if (ImGui::Button("Step 100"))
    {
      atari.execInstructions(100);
    }
    if (ImGui::Button("Step 1000"))
    {
      atari.execInstructions(1000);
    }

    {
      std::string break_str;
      if (ImGui::InputText("break", &break_str, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        try
        {
          int breakpoint = stoi(break_str, nullptr, 16);
          std::cerr << "setting breakpoint at " << std::hex << breakpoint << std::dec << std::endl;
          atari.clearBreakpoints();
          atari.addBreakpoint(breakpoint);
        }
        catch (const std::exception& ex)
        {
          std::cerr << "Cannot convert " << break_str << " to integer  : " << ex.what() << std::endl;
        }
      }
    }

    ImGui::BeginTable("regs", 2, 0, ImVec2(180.0, 300.0), 0.0);
    ImGui::TableSetupColumn("reg_name", ImGuiTableColumnFlags_WidthFixed, 40.0);
    ImGui::TableSetupColumn("reg_value", ImGuiTableColumnFlags_WidthFixed, 100.0);

    auto draw_reg_row = [](const char* name, const char* fmt, unsigned value)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%s", name);
      ImGui::TableNextColumn();
      ImGui::Text(fmt, value);
    };

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("OP");
    ImGui::TableNextColumn();
    ImGui::Text("%s", cpu .getOpName(cpu.instr_[0]));

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("INSTR");
    ImGui::TableNextColumn();
    ss_.str("");
    for (unsigned ii = 0; ii < cpu .instr_len_; ++ii)
    {
      ss_ << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(cpu.instr_[ii]) << ' ';
    }
    ImGui::Text("%s", ss_.str().c_str());

    draw_reg_row("PC", "%04X", cpu.pc_);
    draw_reg_row("SP", "1%02X", cpu.sp_);
    draw_reg_row("A", "%02X", cpu.a_);
    draw_reg_row("X", "%02X", cpu.x_);
    draw_reg_row("Y", "%02X", cpu.y_);
    draw_reg_row("N", "%d", cpu.negative_);
    draw_reg_row("Z", "%d", cpu.zero_);
    draw_reg_row("C", "%d", cpu.carry_);
    draw_reg_row("I", "%d", cpu.irq_disable_);
    draw_reg_row("D", "%d", cpu.decimal_mode_);
    draw_reg_row("V", "%d", cpu.overflow_);
    ImGui::EndTable();

    ImGui::End();
  }
};

class TiaWindow
{
public:
  bool show_ = true;

  void draw(Atari2600 &atari)
  {
    Tia &tia = atari.tia_;
    if (!show_)
    {
      return;
    }

    ImGui::Begin("TIA", &show_); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

    ImGui::BeginTable("regs", 2, 0, ImVec2(180.0, 200.0), 0.0);
    ImGui::TableSetupColumn("reg_name", ImGuiTableColumnFlags_WidthFixed, 80.0);
    ImGui::TableSetupColumn("reg_value", ImGuiTableColumnFlags_WidthFixed, 100.0);

    auto draw_reg_row_num = [](const char *name, const char *fmt, unsigned value)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%s", name);
      ImGui::TableNextColumn();
      ImGui::Text(fmt, value);
    };

    auto draw_reg_row_str = [](const char *name, const char *fmt, const char *str)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%s", name);
      ImGui::TableNextColumn();
      ImGui::Text(fmt, str);
    };

    draw_reg_row_num("Scan X", "%d", tia.scan_x_);
    draw_reg_row_num("Scan Y", "%d", tia.scan_y_);

    draw_reg_row_num("Display X", "%d", tia.scanToDisplayX(tia.scan_x_));
    draw_reg_row_num("Display Y", "%d", tia.scanToDisplayY(tia.scan_y_));

    draw_reg_row_num("Color PF", "%02X", tia.settings_.color_pf);
    draw_reg_row_num("Color BK", "%02X", tia.settings_.color_bk);
    draw_reg_row_num("PF Mask", "%05X", tia.settings_.pf_mask);

    draw_reg_row_num("WSync", "%d", tia.wait_sync_);
    draw_reg_row_num("VSync", "%d", tia.vertical_sync_);

    draw_reg_row_num("P0 X", "%d", tia.position_x_p0_);
    draw_reg_row_num("P1 X", "%d", tia.position_x_p1_);

    draw_reg_row_num("P0", "%02X", tia.settings_.p0_mask);
    draw_reg_row_num("P1", "%02X", tia.settings_.p1_mask);

    ImGui::EndTable();

    ImGui::End();
  }
};

// Generate a empty texture with glGenTextures, bind it with glBindTexture, fill your data in with glTexImage2D
class DisplayWindow
{
public:
  bool initialized_ = false;
  GLuint texture_id_;

  static constexpr int DISPLAY_SIZE_MULT = 3;
  static constexpr int DISPLAY_MULT_WIDTH = Tia::DISPLAY_WIDTH * DISPLAY_SIZE_MULT;
  static constexpr int DISPLAY_MULT_HEIGHT = Tia::DISPLAY_HEIGHT * DISPLAY_SIZE_MULT;
  std::vector<RGBA> display_{DISPLAY_MULT_WIDTH * DISPLAY_MULT_HEIGHT};

  void draw(Atari2600 &atari)
  {
    if (!initialized_)
    {
      init(atari);
    }

    Tia &tia = atari.tia_;

    for (int y = 0; y < DISPLAY_MULT_HEIGHT; ++y)
    {
      int y2 = y / DISPLAY_SIZE_MULT;
      for (int x = 0; x < DISPLAY_MULT_WIDTH; ++x)
      {
        int x2 = x / DISPLAY_SIZE_MULT;
        display_[y * DISPLAY_MULT_WIDTH + x] = tia.getDisplay(x2, y2);
      }
    }

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, DISPLAY_MULT_WIDTH, DISPLAY_MULT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, display_.data());

    auto result = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, DISPLAY_MULT_WIDTH, DISPLAY_MULT_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, display_.data());
    if (result != 0)
    {
      std::cerr << "gluBuild2DMipmaps fail " << result << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);

    float w = 1.2;
    float h = 1.6;
    float x = -0.5 * w;
    float y = -0.5 * h;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(x, y, 0.0f);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(x, y + h, 0.0f);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(x + w, y + h, 0.0f);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(x + w, y, 0.0f);

    glDisable(GL_TEXTURE_2D);

    glEnd();
  }

  void init(Atari2600 &atari)
  {
    initialized_ = true;
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::cout << " Texture ID " << texture_id_ << std::endl;
  }
};

Mos6502Window mos6502_window;
RamWindow ram_window;
DisplayWindow display_window;
TiaWindow tia_window;

void MainLoopStep()
{
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGLUT_NewFrame();
  ImGui::NewFrame();
  ImGuiIO &io = ImGui::GetIO();

  mos6502_window.draw(atari);
  ram_window.draw(atari);
  tia_window.draw(atari);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
  if (false)
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
  // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  // glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.

  // Draw rect to screen
  display_window.draw(atari);

  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

  glutSwapBuffers();
  glutPostRedisplay();
}
