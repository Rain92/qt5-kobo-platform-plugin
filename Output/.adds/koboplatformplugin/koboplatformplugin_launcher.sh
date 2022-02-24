export APPNAME=koboplatformplugin
export ADDSPATH=/mnt/onboard/.adds
export APPPATH=${ADDSPATH}/${APPNAME}
export QTPATH=${ADDSPATH}/qt-linux-5.15-kde-kobo
export QTPLUGINOBENKYOBO=kobo

LOGFILE=${APPPATH}/log.txt

# kill nickel
source ${APPPATH}/exit_nickel.sh

# export QT stuff
export LD_LIBRARY_PATH=${QTPATH}/lib:lib:
export QT_QPA_PLATFORM=${QTPLUGINOBENKYOBO}:

# start APP
${APPPATH}/${APPNAME} > $LOGFILE 2>&1

# restart nickel
sh ${APPPATH}/start_nickel.sh
