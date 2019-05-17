#include <iostream>
#include <fstream>    //fstream
#include <unistd.h> //close
#include <fcntl.h>  //open

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include "google/protobuf/message.h"

#include "proto/fang.pb.h"

using namespace std;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::Message;

const int kProtoReadBytesLimit = INT_MAX;  // Max size of 2 GB minus 1 byte.

bool ReadProtoFromTextFile(const char* filename, Message* proto) {
    int fd = open(filename, O_RDONLY);
    if(fd < 0)
        return false;
    FileInputStream* input = new FileInputStream(fd);
    bool success = google::protobuf::TextFormat::Parse(input, proto);
    delete input;
    close(fd);
    return success;
}

void WriteProtoToTextFile(const Message& proto, const char* filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FileOutputStream* output = new FileOutputStream(fd);
    google::protobuf::TextFormat::Print(proto, output);
    delete output;
    close(fd);
}

bool ReadProtoFromBinaryFile(const char* filename, Message* proto) {
    int fd = open(filename, O_RDONLY);
    if(fd < 0)
        return false;
    ZeroCopyInputStream* raw_input = new FileInputStream(fd);
    CodedInputStream* coded_input = new CodedInputStream(raw_input);
    coded_input->SetTotalBytesLimit(kProtoReadBytesLimit, 536870912);

    bool success = proto->ParseFromCodedStream(coded_input);

    delete coded_input;
    delete raw_input;
    close(fd);
    return success;
}

void WriteProtoToBinaryFile(const Message& proto, const char* filename) {
    fstream output(filename, ios::out | ios::trunc | ios::binary);
    proto.SerializeToOstream(&output);
}

int main()
{
#define BUFSIZE 100
    char name[BUFSIZE]={0};

    fang::AddressBook address_book; //this is a Message struct !

    /*fstream input("./abc.bin", ios::in | ios::binary);
    if (input)
    {
        if (!address_book.ParseFromIstream(&input))
        {
            cerr << "Filed to parse address book." << endl;
            return -1;
        }
    }*/
    if(ReadProtoFromBinaryFile("./abc.bin",&address_book))
    {
        cout<<"already has something"<<endl;
        if(address_book.blist())
            cout<<"blist is true"<<endl;
        else
            cout<<"blist is false"<<endl;
        for(int i=0; i< address_book.persion_size(); i++)
        {
            const fang::Persion& persion = address_book.persion(i);
            cout<<persion.name()<<" "<<persion.age()<<endl;
        }
        cout<<"======================================"<<endl;
    }

    address_book.Clear();
    address_book.set_blist(true);

    for(int i=0;i<10;i++)
    {
        fang::Persion *pPersion = address_book.add_persion();
        snprintf(name,BUFSIZE,"Mr.%d",i);
        pPersion->set_name(name);
        pPersion->set_age(i);
    }

    for(int i=0; i< address_book.persion_size(); i++)
    {
        const fang::Persion& persion = address_book.persion(i);
        cout<<persion.name()<<" "<<persion.age()<<endl;
    }

    /*fstream output("./abc.bin",ios::out | ios::trunc | ios::binary);
    address_book.SerializeToOstream(&output);*/
    WriteProtoToTextFile(address_book, "./abc.txt");
    WriteProtoToBinaryFile(address_book,"./abc.bin");
    return 0;
}
