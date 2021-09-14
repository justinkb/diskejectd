// Disk Claim Daemon
//
// Compile with:
// cc diskclaimd.c -g -o diskclaimd -framework Foundation -framework DiskArbitration

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>

CFStringRef *paths = NULL;
int pathCount = 0;
bool run = true;

void DiskClaimCallback(DADiskRef disk, DADissenterRef dissenter, void *context)
{
    if (dissenter)
        fprintf(stderr, "Failed to claim disk %s\n", DADiskGetBSDName(disk));
}

void DiskAppearedCallback(DADiskRef disk, void *context)
{
    CFDictionaryRef description = DADiskCopyDescription(disk);
    CFStringRef diskMediaPath = description ? CFDictionaryGetValue(description, kDADiskDescriptionMediaPathKey) : NULL;

    if (diskMediaPath)
        for (int i = 0; i < pathCount; i++)
            if (CFEqual(diskMediaPath, paths[i]))
                DADiskClaim(disk, 0, 0, NULL, DiskClaimCallback, NULL);
}

void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGHUP:
        CFRunLoopStop(CFRunLoopGetCurrent());
        break;
    case SIGTERM:
        run = false;
        CFRunLoopStop(CFRunLoopGetCurrent());
        break;
    case SIGINT:
        run = false;
        CFRunLoopStop(CFRunLoopGetCurrent());
        break;
    case SIGQUIT:
        run = false;
        CFRunLoopStop(CFRunLoopGetCurrent());
        break;
    default:
        break;
    }
}

int main(int argc, const char *argv[])
{
    int argi = 1;

    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    DAApprovalSessionRef session = DAApprovalSessionCreate(kCFAllocatorDefault);
    if (!session)
    {
        fprintf(stderr, "Failed to create Disk Arbitration session\n");
    }
    else if (argc - argi <= 0)
    {
        fprintf(stderr, "Usage: %s <path> ...\n", argv[0]);
    }
    else
    {
        CFStringRef cfStringPaths[argc - argi];
        fprintf(stdout, "Will try to claim:\n");
        for (pathCount = 0; pathCount < argc - argi; pathCount++)
        {
            fprintf(stdout, "(%d) %s\n", pathCount + 1, argv[pathCount + argi]);
            cfStringPaths[pathCount] = CFStringCreateWithCStringNoCopy(NULL,
                                                                       argv[pathCount + argi],
                                                                       kCFStringEncodingUTF8,
                                                                       kCFAllocatorNull);
        }
        paths = cfStringPaths;

        DARegisterDiskAppearedCallback(session, NULL, DiskAppearedCallback, NULL);
        DAApprovalSessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

        while (run)
            CFRunLoopRun();

        DAApprovalSessionUnscheduleFromRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        DAUnregisterCallback(session, DiskAppearedCallback, NULL);

        CFRelease(session);
    }
    return 0;
}
