#include <sys/mman.h>
#include <fcntl.h> /* For O_* constants */
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <wchar.h>

using namespace std;

struct LinkedMem {
	uint32_t uiVersion;
	uint32_t uiTick;
	float	fAvatarPosition[3];
	float	fAvatarFront[3];
	float	fAvatarTop[3];
	wchar_t	name[256];
	float	fCameraPosition[3];
	float	fCameraFront[3];
	float	fCameraTop[3];
	wchar_t	identity[256];
	uint32_t context_len;
	unsigned char context[256];
	wchar_t description[2048];
};

class MumbleLink {
    LinkedMem *lm = NULL;
    bool readOnly;
    string memoryTag;

public:
    MumbleLink(string memoryTag, bool readOnly=false) : memoryTag(memoryTag), readOnly(readOnly) {
    }

    ~MumbleLink() {
        lm = NULL;
    }

    void initMumble() {
        char memname[256];
        snprintf(memname, 256, "/%s.%d", memoryTag.c_str(), getuid());

        int shmfd = shm_open(memname, readOnly ? O_RDONLY : O_RDWR, S_IRUSR | S_IWUSR);

        if (shmfd < 0) {
            int errsv = errno;
            cout << "shm_open failed, errno " << errsv << "\n";
            cout << "See `man shm_open' for a list of error codes\n";
            return;
        }

        lm = (LinkedMem *)(mmap(NULL, sizeof(struct LinkedMem), PROT_READ | (readOnly ? 0 : PROT_WRITE), MAP_SHARED, shmfd,0));

        if (lm == (void *)(-1)) {
            int errsv = errno;
            cout << "mmap failed, errno " << errsv << "\n";
            cout << "See `man mmap' for a list of error codes\n";
            lm = NULL;
            return;
        }
    }

    void sync(const MumbleLink& other) {
        if (readOnly) {
            cout << "Link is read-only\n";
            return;
        }
        if (!lm)
        {
            cout << "Not initialized\n";
            return;
        }

        if(lm->uiVersion != 2 && other.lm->uiVersion == 2) {
            wcsncpy(lm->name, other.lm->name, 256);
            wcsncpy(lm->description, other.lm->description, 2048);
            lm->uiVersion = 2;
        }
        lm->uiTick = other.lm->uiTick;

        // Unit vector pointing out of the avatar's eyes aka "At"-vector.
        lm->fAvatarFront[0] = other.lm->fAvatarFront[0];
        lm->fAvatarFront[1] = other.lm->fAvatarFront[1];
        lm->fAvatarFront[2] = other.lm->fAvatarFront[2];

        // Unit vector pointing out of the top of the avatar's head aka "Up"-vector (here Top points straight up).
        lm->fAvatarTop[0] = other.lm->fAvatarTop[0];
        lm->fAvatarTop[1] = other.lm->fAvatarTop[1];
        lm->fAvatarTop[2] = other.lm->fAvatarTop[2];

        // Position of the avatar (here standing slightly off the origin)
        lm->fAvatarPosition[0] = other.lm->fAvatarPosition[0];
        lm->fAvatarPosition[1] = other.lm->fAvatarPosition[1];
        lm->fAvatarPosition[2] = other.lm->fAvatarPosition[2];

        // Same as avatar but for the camera.
        lm->fCameraPosition[0] = other.lm->fCameraPosition[0];
        lm->fCameraPosition[1] = other.lm->fCameraPosition[1];
        lm->fCameraPosition[2] = other.lm->fCameraPosition[2];

        lm->fCameraFront[0] = other.lm->fCameraFront[0];
        lm->fCameraFront[1] = other.lm->fCameraFront[1];
        lm->fCameraFront[2] = other.lm->fCameraFront[2];

        lm->fCameraTop[0] = other.lm->fCameraTop[0];
        lm->fCameraTop[1] = other.lm->fCameraTop[1];
        lm->fCameraTop[2] = other.lm->fCameraTop[2];

        wcsncpy(lm->identity, other.lm->identity, 256);
        memcpy(lm->context, other.lm->context, other.lm->context_len);
        lm->context_len = other.lm->context_len;
    }

    void display() {
        if (!lm)
        {
            cout << "Not initialized\n";
            return;
        }

        wcout << L"name: " << lm->name << "\n";
        wcout << L"description: " << lm->description << "\n";
        cout << "ui version: " << lm->uiVersion << "\n";
        cout << "tick: " << lm->uiTick << "\n";

        cout << "context (len "<< lm->context_len << "): " << lm->context << "\n";
        wcout << L"identity: " << lm->identity << "\n";

        cout << "avatar position: (";
        cout << lm->fAvatarPosition[0] << ", ";
        cout << lm->fAvatarPosition[1] << ", ";
        cout << lm->fAvatarPosition[2] << ")\n";

        cout << "avatar front: (";
        cout << lm->fAvatarFront[0] << ", ";
        cout << lm->fAvatarFront[1] << ", ";
        cout << lm->fAvatarFront[2] << ")\n";

        cout << "avatar top: (";
        cout << lm->fAvatarTop[0] << ", ";
        cout << lm->fAvatarTop[1] << ", ";
        cout << lm->fAvatarTop[2] << ")\n";

        cout << "camera position: (";
        cout << lm->fCameraPosition[0] << ", ";
        cout << lm->fCameraPosition[1] << ", ";
        cout << lm->fCameraPosition[2] << ")\n";

        cout << "camera front: (";
        cout << lm->fCameraFront[0] << ", ";
        cout << lm->fCameraFront[1] << ", ";
        cout << lm->fCameraFront[2] << ")\n";

        cout << "camera top: (";
        cout << lm->fCameraTop[0] << ", ";
        cout << lm->fCameraTop[1] << ", ";
        cout << lm->fCameraTop[2] << ")\n";
    }
};


int main()
{
    cout << "Initializing\n";
    MumbleLink readLink = MumbleLink("PyMumbleLink", true);
    MumbleLink writeLink = MumbleLink("MumbleLink");
    readLink.initMumble();
    writeLink.initMumble();
    writeLink.sync(readLink);
    writeLink.display();
}
