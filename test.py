import sys

def ag_process(inputdata):
    if len(inputdata) > 0:
        # problem detected print information and exit with error code
        print(inputdata[0])
        return 1

    # all good, quit with success
    return 0


sys.exit(ag_process([[float(c) for c in x.split(':')] for x in sys.argv[1:]]))
