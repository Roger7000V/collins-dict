#if defined __APPLE__ || defined __unix__
#include <sys/ioctl.h>
#include <unistd.h>
#elif _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#define MAX_COL 77;

#include <cpr/cpr.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/dom.h>

#include <iomanip>
#include <iostream>
#include <string>

using std::cerr;
using std::cout, std::endl;
using std::string;

enum selection { CLASS, TAG };

void check_status(const lxb_status_t& status, const string& err_msg)
{
    if (status != LXB_STATUS_OK) {
        cerr << err_msg << endl;
        exit(EXIT_FAILURE);
    }
}

lxb_dom_collection_t* get_collection(lxb_html_document_t* document)
{
    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 10);
    if (collection == nullptr) {
        cerr << "error: Failed to create collection data" << endl;
        exit(EXIT_FAILURE);
    }
    return collection;
}

// Get the first element
lxb_dom_element_t* get_element(const selection& option, lxb_html_document_t* document,
    lxb_dom_element_t* parent, const string& name)
{
    lxb_dom_collection_t* collection = get_collection(document);

    lxb_status_t status;
    switch (option) {
        case CLASS:
            status = lxb_dom_elements_by_class_name(parent, collection
                , reinterpret_cast<const lxb_char_t* >(name.c_str()), name.size());
            break;
        case TAG:
            status = lxb_dom_elements_by_tag_name(parent, collection
                , reinterpret_cast<const lxb_char_t* >(name.c_str()), name.size());
            break;
        default:
            cerr << "error: Unknown selection" << endl;
            exit(EXIT_FAILURE);
    }
    check_status(status, "error: Failed to get data");

    if (lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return nullptr;
    }
    lxb_dom_element_t* data = lxb_dom_collection_element(collection, 0);
    lxb_dom_collection_destroy(collection, true);
    return data;
}

lxb_dom_element_t* get_element_by_class(lxb_html_document_t* document, lxb_dom_element_t* parent,
    const string& name)
{
    return get_element(selection{CLASS}, document, parent, name);
}

lxb_dom_element_t* get_element_by_tag(lxb_html_document_t* document, lxb_dom_element_t* parent,
    const string& name)
{
    return get_element(selection{TAG}, document, parent, name);
}

const lxb_char_t* get_text(const lxb_dom_element_t* element)
{
    return lxb_dom_node_text_content(lxb_dom_interface_node(element), nullptr);
}

inline string cast_string(const lxb_char_t* str)
{
    return string{reinterpret_cast<const char* >(str)};
}

string& format(string& str)
{
    unsigned long i = 0;
    while (i < str.length()) {
        if (!std::isspace(str[i])) {
            break;
        }
        ++i;
    }
    unsigned long j = str.length();
    while (j > 0) {
        if (!std::isspace(str[j])) {
            break;
        }
        --j;
    }
    str = str.substr(i, j);
    string res;
    i = 0;
    while (i < str.length()) {
        if (std::isspace(str[i])) {
            res += ' ';
            do
                ++i;
            while (std::isspace(str[i]));
        } else {
            res += str[i];
            ++i;
        }
    }
    str = res;
    return str;
}

void get_synonym(lxb_html_document_t* document, lxb_dom_element_t* data)
{
    lxb_dom_element_t* field = get_element_by_class(document, data, "thes");
    if (field != nullptr) {
        cout << "    Synonyms: ";

        lxb_dom_collection_t* collection = get_collection(document);
        lxb_status_t status = lxb_dom_elements_by_class_name(field, collection
            , reinterpret_cast<const lxb_char_t* >("ref"), 3);
        check_status(status, "error: Failed to get data");
        cout << get_text(lxb_dom_collection_element(collection, 0));
        for (unsigned int j = 1; j < lxb_dom_collection_length(collection) - 1; ++j) {
            cout << ", " << get_text(lxb_dom_collection_element(collection, j));
        }
        cout << endl;
        lxb_dom_collection_destroy(collection, true);
    }
}


unsigned short get_terminal_width()
{
#if defined __APPLE__ || defined __unix__
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 3 ? w.ws_col - 3 : MAX_COL;
#elif _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    auto width = csbi.srWindow.Right - csbi.srWindow.Left - 1;
    return width > 3 ? width - 3 : MAX_COL;
#else
    return MAX_COL;
#endif
}

void wrap_line(const string& text, const unsigned int width)
{
    const string indent{"   "};

    std::istringstream stream{text};
    string word;
    unsigned int written_length{0};

    cout << indent;
    while (stream >> word) {
        if (word.size() + written_length > width) {
            cout << '\n' << indent;
            written_length = 0;
        }
        cout << ' ' << word;
        written_length += word.size() + 1;
    }
    cout << endl;
}

int main(const int argc, const char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc != 2) {
        cerr << "error: Incorrect number of arguments" << endl;
        cerr << "usage: dict <word/phrase>" << endl;
        return EXIT_FAILURE;
    }
    string lookup{argv[1]};
    const cpr::Response r = cpr::Get(cpr::Url{"https://www.collinsdictionary.com/us/search/"},
        cpr::Parameters({{"dictCode", "english"}, {"q", lookup}}),
        cpr::Header{{"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:10.0)"
            " Gecko/20100101 Firefox/10.0}"}});

    if (r.status_code != 200) {
        cerr << "error: Failed to get web content" << endl;
        return 0;
    }
    string text = r.text;
    lxb_status_t status;

    auto* html = reinterpret_cast<const lxb_char_t* >(text.c_str());
    lxb_html_document_t* document = lxb_html_document_create();
    status = lxb_html_document_parse(document, html, text.length() - 1);
    check_status(status, "Failed to parse webpage");

    // Get Collins default dictionary
    lxb_dom_element_t* data = lxb_dom_interface_element(document->body);
    lxb_dom_element_t* dict = get_element_by_class(document, data, "cobuild");

    // The word does not exist in dictionary
    if (dict == nullptr) {
        dict = get_element_by_class(document, data, "american");
        if (dict == nullptr) {
            cout << "No word or phrase named \"" << lookup << '"' << endl;

            data = get_element_by_class(document, data, "suggested_words");
            if (data != nullptr) {
                lxb_dom_collection_t* collection = get_collection(document);
                status = lxb_dom_elements_by_tag_name(data, collection
                    , reinterpret_cast<const lxb_char_t* >("li"), 2);
                check_status(status, "Failed to get data");
                cout << "Maybe you mean:" << endl;
                // Print in two columns
                auto size = static_cast<unsigned int>(lxb_dom_collection_length(collection));
                unsigned int half = size / 2;
                for (unsigned int i = 0; i < half; ++i) {
                    cout << std::left << std::setw(45)
                              << get_text(lxb_dom_collection_element(collection, i));
                    cout << get_text(lxb_dom_collection_element(collection, half + i));
                    cout << endl;
                }
                if (size % 2 == 1) {
                    cout << get_text(lxb_dom_collection_element(collection, half)) << endl;
                }
                lxb_dom_collection_destroy(collection, true);
            }
            lxb_html_document_destroy(document);
            return 0;
        }
    }

    // Get word
    data = get_element_by_tag(document, dict, "h2");
    cout << get_text(data);

    // Get pronunciation
    data = get_element_by_class(document, dict, "pron");
    if (data != nullptr) {
        string pron{reinterpret_cast<const char* >(get_text(data))};
        pron = pron.substr(0, pron.length() - 3);
        cout << " [" << pron << ']';
    }
    cout << endl;

    // Get word forms
    data = get_element_by_class(document, dict, "type-infl");
    if (data != nullptr) {
        cout << get_text(data) << endl;
    }

    // Get types
    lxb_dom_collection_t* collection = get_collection(document);
    status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(dict), collection
        , reinterpret_cast<const lxb_char_t* >("hom"), 3);
    check_status(status, "Failed to get data");
    for (unsigned int i = 0; i < lxb_dom_collection_length(collection); ++i) {
        data = lxb_dom_collection_element(collection, i);

        lxb_dom_element_t* field;

        // Get numbering
        field = get_element_by_class(document, data, "sensenum");
        if (field != nullptr) {
            string num = reinterpret_cast<const char* >(get_text(field));
            cout << '\n' << num.substr(0, num.length() - 2);
        }

        // Get grammar group
        field = get_element_by_class(document, data, "gramGrp");
        if (field != nullptr) {
            if (lxb_dom_collection_length(collection) != 1) {
                cout << ' ';
            }
            string gram_grp = cast_string(get_text(field));
            if (gram_grp[0] == ' ') {
                cout << gram_grp.substr(1, gram_grp.length()) << endl;
            } else {
                cout << gram_grp << endl;
            }
        }

        const auto w = get_terminal_width();

        // Get definition
        field = get_element_by_class(document, data, "def");
        if (field != nullptr) {
            string def = cast_string((get_text(field)));
            wrap_line(def, w);
        } else {
            field = get_element_by_class(document, data, "xr");
            string def = cast_string((get_text(field)));
            cout << ' ' << format(def) << endl;
        }

        // Get example
        field = get_element_by_class(document, data, "quote");
        if (field != nullptr) {
            string example = cast_string((get_text(field)));
            cout << '\n';
            wrap_line("-> " + example.substr(1), w);
        }

        // Get synonym
        get_synonym(document, data);

        // Get derivation
        lxb_dom_collection_t* drv_collection = get_collection(document);
        status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(data), drv_collection
            , reinterpret_cast<const lxb_char_t* >("type-drv"), 8);
        check_status(status, "Failed to get data");
        if (lxb_dom_collection_length(drv_collection) != 0) {
            cout << "\n    Derivations:" << endl;
        }
        for (unsigned int j = 0; j < lxb_dom_collection_length(drv_collection); ++j) {
            if (j % 2 == 1) {
                continue;
            }
            field = lxb_dom_collection_element(drv_collection, j);
            string grp = cast_string(get_text(get_element_by_class(document, field, "gramGrp")));
            cout << "    - " << get_text(get_element_by_class(document, field, "orth"))
                      << " (" << grp.substr(1, grp.length()) << ")" << endl;
            string example = cast_string(get_text(get_element_by_class(document, field, "quote")));
            cout << '\n';
            wrap_line("-> " + example.substr(1), w);

            // Get synonym
            get_synonym(document, field);
        }
        lxb_dom_collection_destroy(drv_collection, true);
    }
    lxb_dom_collection_destroy(collection, true);
    lxb_html_document_destroy(document);
    return 0;
}
