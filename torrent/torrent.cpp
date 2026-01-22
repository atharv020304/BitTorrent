#include "torrent.hpp"
#include "sha1.hpp"

TorrentMeta parseTorrent(const string &path)
{
    TorrentMeta  t;

    string raw = readFile(path);
    BPtr root = bdecode(raw);

    t.announce = dict_get(root,"announce")->s;

    BPtr infoDict = dict_get(root,"info");

    string encodedInfo = bencode(infoDict);
    t.info_hash = sha1(encodedInfo);

    t.name = dict_get(infoDict,"name")->s;
    t.length = dict_get(infoDict, "length")->ival;

    return t;
}


//main func for test
int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " file.torrent\n";
        return 1;
    }

    try {
        TorrentMeta meta = parseTorrent(argv[1]);

        cout << "Torrent parsed successfully\n";
        cout << "---------------------------\n";
        cout << "Name       : " << meta.name << "\n";
        cout << "Announce   : " << meta.announce << "\n";
        cout << "Total size : " << meta.length << " bytes\n";
        cout << "Info hash  : " << meta.info_hash << "\n";

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
