set(qtproject_SRCS
	main.cpp    
    utils.cpp
    NarcissisticNumCalculator.cpp
    NarcissisticNumbers.cpp
)

set(qtproject_H
    NarcissisticNumbers.h
)

set(project_H
    NarcissisticNumCalculator.h
    utils.h
)

set(qtproject_UIS
    NarcissisticNumbers.ui
)


set(qtproject_QRC
	application.qrc
)

file(GLOB qtproject_QRC_SOURCES "resources/*")
