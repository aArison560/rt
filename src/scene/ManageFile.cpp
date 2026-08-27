// #include "scene/ManageFile.hpp"
#include "../../include/scene/ManageFile.hpp"

ManageFile::ManageFile( int argc, char **argv )
{
    this->argc = argc;
    this->argv = argv;
}

ManageFile::ManageFile( const ManageFile &cp )
{
    *this = cp;
}

ManageFile & ManageFile::operator= (const ManageFile &aff)
{
    if (this == &aff)
        return *this;
    this->argc = aff.argc;
    this->argv = aff.argv;
    return *this;
}

bool ManageFile::valueRbgIsRight(float r, float g, float b)
{
    bool br = r >= 0.0 && r <= 1.0;
    bool bg = g >= 0.0 && g <= 1.0;
    bool bb = b >= 0.0 && b <= 1.0;
    return (br && bg && bb);
}

bool ManageFile::valueIsShininess(float v)
{
    return (v >= 0.0 && v <= 128.0);
}

bool ManageFile::rgbAllValueIsRight(std::string key, std::vector<float> vec)
{
    if (key == "bg" && !this->valueRbgIsRight(vec.at(0), vec.at(1), vec.at(2)))
        return false;
    if (key == "A")
    {
        if (vec.size() == 3 && !this->valueRbgIsRight(vec.at(0), vec.at(1), vec.at(2)))
            return false;
        else if (vec.size() == 1 && !this->valueRbgIsRight(vec.at(0), 0.0f, 0.0f))
            return false;
    }
    if ((key == "L" || key == "directional") && (!this->valueRbgIsRight(vec.at(3), vec.at(4), vec.at(5)) \
        || !this->valueRbgIsRight(vec.at(6), 0.0, 0.0)))
        return false;
    if (key == "material")
    {
        if (!this->valueRbgIsRight(vec.at(0), vec.at(1), vec.at(2)) || !this->valueRbgIsRight(vec.at(3), 0.0f, 0.0f) \
            || !this->valueRbgIsRight(vec.at(4), 0.0f, 0.0f) || !!this->valueRbgIsRight(vec.at(5), 0.0f, 0.0f) \
            || !this->valueIsShininess(vec.at(6)) || !this->valueRbgIsRight(vec.at(7), 0.0f, 0.0f))
            return false;
    }
    return true;
}

bool    ManageFile::verifyElem( std::vector<std::string> scene )
{
    std::map<std::string, std::array<int, 3>> elem;
    std::vector<float> tab;
    float num = 0.0;
    int count = 0;

    elem["bg"] = {-1, -1, 3};
    elem["A"] = {-1, 1, 3};
    elem["L"] = {-1, -1, 7};
    elem["directional"] = {-1, -1, 7};
    elem["c"] = {5, 7, 10};
    elem["sp"] = {-1, -1, 4};
    elem["pl"] = {-1, -1, 6};
    elem["cy"] = {-1, -1, 8};
    elem["co"] = {-1, -1, 8};
    elem["material"] = {-1, -1, 8};

    for (std::vector<std::string>::iterator it = scene.begin(); it != scene.end(); it++)
    {
        std::string word = "";
        std::string key = "";
        std::stringstream ss(*it);

        while(ss >> word)
        {
            try {
                if (key.empty())
                    key = word;
                else
                {
                    size_t processedChars = 0;
                    num = std::stof(word, &processedChars);
                    if (processedChars != word.length())
                        throw std::invalid_argument("");
                    tab.push_back(num);
                    count++;
                }
            }
            catch (const std::invalid_argument & e) {
                std::cerr << "Error: '" << word << "' is not a valid number." << std::endl;
                return false;
            }
            catch (const std::out_of_range & e) {
                std::cerr << " Error: '" << word << "' is out of float range." << std::endl;
                return false;
            }
        }
        std::cout << "key: " << key << "| count: " << count << std::endl;
        if (std::find(elem[key].begin(), elem[key].end(), count) == elem[key].end())
        {
            std::cerr << "Invalid syntaxe or parameters in key("<< key << ")" << std::endl;
            return false;
        }
        if (key == "bg" || key == "A" || key == "L" || key == "directional" || key == "material")
        {
            if (!this->rgbAllValueIsRight(key, tab))
            {
                std::cerr << "Invalid syntaxe or parameters in key("<< key << ")" << std::endl;
                return false;
            }
        }
        count = 0;
        word = "";
        key = "";
        tab.clear();
    }
    return true;
}

void    ManageFile::checkErrorFile( void )
{
    if (this->argc != 2)
        throw std::invalid_argument("Arguments of file error, the number parm must be 1");
    std::ifstream fd(this->argv[1]);
    std::string buff;
    std::vector<std::string> scene;

    while(std::getline(fd, buff))
    {
        if (!buff.empty() && buff.at(0) != '#')
            scene.push_back(buff);
        buff = "";
    }
    if (scene.size() == 0)
    {
        std::cerr << "Empty file..." << std::endl;
        throw std::logic_error("");

    }
    if (!this->verifyElem(scene))
        throw std::invalid_argument("");
}

ManageFile::~ManageFile( void )
{
    return ;
}
