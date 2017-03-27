#!/usr/local/bin/python
# this function iterates over values of one camera coniguration and captures
# an image each time, while possible also setting another to a constant value for
# the whole run, call it as follows:
# ./exposure_series.py [varied configuration option] [range start] [range end] [range increment] [constant configuration option] [constant value]
# for example: ./exposure_series.py shutterspeed 20 50 2 f-number 0
# Email : kalpana.singh@ualberta.ca

import couchdb
import sys
import os.path
import capture
from subprocess import call

def connectToDB(dbName):
    status = "ok"
    db = {}
    try:
        db = couch[dbName]
    except:
        print "Failed to connect to database:" + dbName
        status = "bad"
    return status, db


if __name__ == "__main__":
    #take series parametes as arguments
    if len(sys.argv)<5:
        sys.exit(-1)
    else:
        seriesType=sys.argv[1]
        seriesStart=sys.argv[2]
        seriesEnd=sys.argv[3]
        seriesIncrement=sys.arvv[4]
    if len(sys.argv)>=7:
        constType=sys.argv[5]
        constValue=sys.argv[6]
        call(['./config_all.sh',constType,constValue])
        constString="_"+constType+"="+constValue
    else:
        constString=""
    
    #attempt to capture images
    for seriesNum in range(int(seriesStart),int(seriesEnd)+1,seriesIncrement):
        call(['./config_all.sh',seriesType,str(seriesNum)])
        saveName=seriesType+"="+str(seriesNum)+constString
        errorFlag,captureTime,captureFolder, activeCams=capture.py_capture("exposure_tests")
        if(errorFlag!=0):
            sys.exit(errorFlag)
        runDescription=seriesType+"="+str(seriesNum)+constString
        
        #upload captured images and logfiles to database
        dbName='image_storage_test'
        #arguments to be passed from capture executable
        #open couchdb server and database where images from capture are to be posted
        couch = couchdb.Server()
        dbstatus, db = connectToDB(dbName)
        #create a new document for the capture and write captureTime and runNumber to it (run number will have to be passed to this script somehow)
        #note that timestamp is the capture time for the images and not the database upload time (although they should presumably be close
        if(dbstatus is "ok"):
            doc_id, doc_rev = db.save({'timestamp':int(captureTime),'run_number':0,'run_description':runDescription, 'active_cameras':activeCams,'log':'NA'})
            doc=db[doc_id]
            #search captureFolder for logfiles and image files and upload to database as attatchments
            for filename in os.listdir (captureFolder):
                if filename.lower().endswith('.nef'):
                    image=open(captureFolder+filename)
                    db.put_attachment(doc,image,None,'image/x-nikon-nef')
                elif filename.lower().endswith('.jpg'):
                    image=open(captureFolder+filename)
                    db.put_attachment(doc,image,None,'image/jpg')
                elif filename.lower().endswith('.log'):
                    logfile=open(captureFolder+filename)
                    try:
                        logstring=logfile.read()
                        doc=db[doc_id]
                        doc['log']=logstring
                        db[doc_id]=doc
                    except:
                        sys.exit(12)
        else:
            sys.exit(11)
    sys.exit(0)
