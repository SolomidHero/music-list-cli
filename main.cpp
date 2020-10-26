#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <string>
#include <vector>
#include <map>

// util functions to read

// read all file
// ref: https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
const std::string readfile(const std::string& path) {
  std::ifstream fin(path);
  std::string results;

  fin.seekg(0, std::ios::end);   
  results.reserve(fin.tellg());
  fin.seekg(0, std::ios::beg);

  results.assign((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

  fin.close();
  return results;
}

// read all input from stdin
// ref: https://stackoverflow.com/questions/201992/how-to-read-until-eof-from-cin-in-c
// double '\n' to end
const std::string readistream(std::istream& is) {
  std::string results = "";
  std::string line;
  int leave = 0;
  is.ignore();

  while (leave < 2) {
    std::getline(is, line);
    results += line + '\n';
    if (line == "") {
      ++leave;
    } else {
      leave = 0;
    }
  }

  return results;
}

class Song {
 public:
  Song(std::string& name, std::string author,
    const int year, const std::string lyrics="")
    : name(name), author(author), year(year), lyrics(lyrics) {}

  // editing
  void clear_lyrics() { lyrics = ""; }
  void edit_lyrics(std::string&& text) { lyrics = text; }
  void save(const std::string& path) {
    std::ofstream fout(path);
    fout << lyrics;
    fout.close();
  }

  // navigating
  const std::string get_author() { return author; }
  const std::string get_lyrics() { return lyrics; }
  const std::string info() {
    std::stringstream os;
    os << author << " - " << name << get_year();
    return os.str();
  }

  friend std::ostream& operator<<(std::ostream& os, const Song& song);

 private:
  std::string get_year() const {
    std::string str_year = "";
    if (year >= 0) {
      str_year += " (" + std::to_string(year) + ")";
    }
    return str_year;
  }

 private:
  std::string name;
  std::string author;
  std::string lyrics;
  int year;
};

std::ostream& operator<<(std::ostream& os, const Song& song) {
    os
      << "Song: " << song.name << song.get_year() << std::endl
      << "Lyrics by " << song.author << std::endl
      << "Lyrics:" << std::endl
      << song.lyrics << std::endl;

    return os;
  }

class Library {
 public:
  Library() {}
  Library(std::vector<Song>&& list) : songList(list), browsing_song_id(-1) {}

  std::string info();
  std::string info_by_ids(const std::vector<int>& ids);
  bool empty();

  // editing
  void add(Song song);

  // search
  const std::vector<int> search_by_author(const std::string& author);
  const std::vector<int> search_by_word(const std::string& word);

  // navigating
  Song& browse(size_t i);

 private:
  std::string _info(std::vector<Song>&);

 private:
  std::vector<Song> songList;

  // Song which we are staring at in this library
  // Operations on song go through library, thus it is good design
  int browsing_song_id;
};

std::string Library::_info(std::vector<Song>& list) {
  std::stringstream desc;

  desc << "There are " << list.size() << " songs found " << std::endl;

  for (int i = 0; i < list.size(); ++i) {
    desc << i + 1 << ". " << list[i].info() << std::endl;
  }

  return desc.str();
}

std::string Library::info() {
  return _info(songList);
}

std::string Library::info_by_ids(const std::vector<int>& ids) {
  std::stringstream desc;

  desc << "There are " << ids.size() << " songs found " << std::endl;

  for (int i = 0; i < ids.size(); ++i) {
    desc << i + 1 << ". " << songList[ids[i]].info() << std::endl;
  }

  return desc.str();
}

bool Library::empty() {
  return songList.empty();
}

void Library::add(Song song) {
  songList.push_back(std::move(song));
}

Song& Library::browse(size_t i) {
  if (i > songList.size()) i = songList.size();
  if (i < 1) i = 1;

  return songList[i - 1];
}

const std::vector<int> Library::search_by_author(const std::string& author) {
  std::vector<int> ids = {};

  for (int i = 0; i < songList.size(); ++i) {
    if (songList[i].get_author() == author) {
      ids.push_back(i);
    }
  }

  return ids;
}

const std::vector<int> Library::search_by_word(const std::string& word) {
  std::vector<int> ids = {};

  for (int i = 0; i < songList.size(); ++i) {
    // checking for substring
    // https://stackoverflow.com/questions/2340281/check-if-a-string-contains-a-string-in-c
    if (songList[i].get_lyrics().find(word) != std::string::npos) {
      ids.push_back(i);
    }
  }

  return ids;
}

class SongsMenu {
 private:
  struct MenuOption {
    std::string description;
    std::function<void(void)> func;
  };

  // types of jobs
  static int n_states;
  enum MenuState {
    Main, Add, ShowAll, View, Delete, Edit, Save, Lyrics,
    SearchAuthor, SearchByWord, Keyboard, File, Exit
  };
 public:
  SongsMenu(std::ostream& os=std::cout, std::istream& is=std::cin)
    : state(Main), library(Library()), os(os), is(is) {

    // passing member functions as arguments
    // ref: https://stackoverflow.com/questions/7582546/using-generic-stdfunction-objects-with-member-functions-in-one-class
    options = {
      { Main, { "Back to main menu", std::bind(&SongsMenu::_main, this) }},
      { Add, { "Add song to the library", std::bind(&SongsMenu::_add, this) }},
      { ShowAll, { "Show all songs in library", std::bind(&SongsMenu::_show, this) }},
      { View, { "Browsing particle song", std::bind(&SongsMenu::_view, this) }},
      { SearchAuthor, { "Search songs of one author", std::bind(&SongsMenu::_search_by_author, this) }},
      { SearchByWord, { "Find song with such word", std::bind(&SongsMenu::_search_by_word, this) }},
      { Exit, { "Exit", std::bind(&SongsMenu::_exit, this) }},
      { Delete, { "Delete song lyrics", std::bind(&SongsMenu::_delete, this) }},
      { Edit, { "Edit song lyrics", std::bind(&SongsMenu::_edit, this) }},
      { Lyrics, { "View song information and lyrics", std::bind(&SongsMenu::_lyrics, this) }},
      { Save, { "Save song lyrics to the disk", std::bind(&SongsMenu::_save, this) }},
      { Keyboard, { "Using keyboard input", nullptr }},
      { File, { "Using file path", nullptr }},
    };
  }

  // main process for menu
  // here we are asking for next user actions
  void run();

 private:
  void _display_options(const std::vector<MenuState>&);
  MenuState _read_state(const std::vector<MenuState>&);

  // implemented below
  void _main();
  void _add();
  void _show();
  void _view();
  void _search_by_author();
  void _search_by_word();
  void _exit();

  void _delete();
  void _edit();
  void _lyrics();
  void _save();

 private:
  Library library;
  MenuState state;
  int cur_song_index;

  // std::vector<MenuOption> options;
  std::map<MenuState, MenuOption> options;

  // stream for communication
  std::ostream& os;
  std::istream& is;
};

void SongsMenu::run() {
  _main();

  while (state != Exit) {
    options[state].func();
    // os.flush();
  }
}

void SongsMenu::_display_options(const std::vector<MenuState>& states) {
  for (int i = 0; i < states.size(); ++i) {
    auto& option = options[states[i]];
    os << i + 1 << ". " << option.description << std::endl;
  }
}

SongsMenu::MenuState SongsMenu::_read_state(const std::vector<MenuState>& states) {
  int state_int;
  is >> state_int;
  assert(state_int > 0 && state_int <= states.size());

  return static_cast<MenuState>(states[state_int - 1]);
}

// menu navigating part

void SongsMenu::_main() {
  // states for this function
  std::vector<MenuState> states = {
    Add, ShowAll, Save,
    SearchAuthor, SearchByWord, Exit
  };
  _display_options(states);
  state = _read_state(states);
}

void SongsMenu::_add() {
  std::vector<MenuState> states = {
    Keyboard, File
  };
  std::string name, author, lyrics, year;

  os << "Please enter:" << std::endl;

  os << "Song name: ";
  is >> name;

  os << "Author: ";
  is >> author;

  os << "Year ('-' if no): ";
  is >> year;

  os << "Lyrics: " << std::endl;
  _display_options(states);
  if (_read_state(states) == Keyboard) {
    lyrics = readistream(is);
  } else {
    std::string path;
    is >> path;
    lyrics = readfile(path);
  }

  library.add(Song(name, author, ((year == "-")? -1 : std::stoi(year)), lyrics));
  state = Main;
}

void SongsMenu::_show() {
  os << library.info() << std::endl;

  int song_index;
  is >> song_index;
  if (song_index == 0 || library.empty()) {
    state = Main;
    return;
  }
  cur_song_index = song_index;
  state = View;
}

void SongsMenu::_view() {
  std::vector<MenuState> states = {
    Delete, Edit, Save, Lyrics, Main
  };
  auto& song = library.browse(cur_song_index);
  os << "Song: " << song.info() << std::endl;

  _display_options(states);
  state = _read_state(states);
}

void SongsMenu::_search_by_author() {
  std::string author;
  os << "Author to search: ";
  is >> author;
  auto ids = library.search_by_author(author);

  os << library.info_by_ids(ids) << std::endl;

  int song_index;
  is >> song_index;
  if (song_index == 0 || song_index > ids.size() || ids.empty()) {
    state = Main;
    return;
  }

  cur_song_index = ids[song_index - 1];
  state = View;
}

void SongsMenu::_search_by_word() {
  std::string word;
  os << "Word to search in songs lyrics: ";
  is >> word;
  auto ids = library.search_by_word(word);

  os << library.info_by_ids(ids) << std::endl;

  int song_index;
  is >> song_index;
  if (song_index == 0 || song_index > ids.size() || ids.empty()) {
    state = Main;
    return;
  }

  cur_song_index = ids[song_index - 1];
  state = View;
}

void SongsMenu::_exit() {
  state = Exit;
}

// editing songs part

void SongsMenu::_delete() {
  auto& song = library.browse(cur_song_index);
  song.clear_lyrics();

  os << "Lyrics for (" << song.info() << ") erased" << std::endl;
  os << "\nPress enter to continue" << std::endl;
  is.get();

  state = View;
}

void SongsMenu::_edit() {
  auto& song = library.browse(cur_song_index);
  std::string lyrics;

  os << "Lyrics: " << std::endl;
  _display_options({ Keyboard, File });
  if (_read_state({ Keyboard, File }) == Keyboard) {
    lyrics = readistream(is);
  } else {
    std::string path;
    is >> path;
    lyrics = readfile(path);
  }

  song.edit_lyrics(std::move(lyrics));
  os << "Lyrics for (" << song.info() << ") changed" << std::endl;
  os << "\nPress enter to continue" << std::endl;
  is.get();

  state = View;
}

void SongsMenu::_lyrics() {
  auto& song = library.browse(cur_song_index);

  os << song << std::endl;
  os << "\nPress enter to continue\n" << std::endl;
  is.get();

  state = View;
}

void SongsMenu::_save() {
  auto& song = library.browse(cur_song_index);
  std::string path;

  os << "Enter path to save lyrics:" << std::endl;
  is >> path;

  song.save(path);
  os << "Lyrics for (" << song.info() << ") saved into " << path << std::endl;
  os << "\nPress enter to continue" << std::endl;
  is.get();

  state = View;
}

int main(int argc, const char** argv) {
  SongsMenu menu;

  menu.run();

  // std::string s1, s2;
  // std::cin >> s1 >> s2;
  // Song a = { s1, s2 };
  // std::cout << a << std::endl;

  return 0;
}