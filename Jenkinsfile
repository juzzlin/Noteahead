pipeline {
    agent any
    stages {
        stage('Build and run tests') {
            matrix {
                agent {
                    docker {
                        image 'juzzlin/qt6-24.10:latest'
                        args '--privileged -t -v $WORKSPACE:/noteahead'
                    }
                }
                axes {
                    axis {
                        name 'BUILD_TYPE'
                        values 'Debug', 'Release'
                    }
                }
                stages {
                    stage('Build and test') {
                        steps {
                            sh "mkdir -p build-$BUILD_TYPE && cd build-$BUILD_TYPE && cmake -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && cmake --build . && ctest"
                        }
                    }
                }
            }
        }
        stage('Source .tar.gz') {
            agent {
                docker {
                    image 'juzzlin/qt6-24.10:latest'
                    args '--privileged -t -v $WORKSPACE:/noteahead'
                }
            }
            steps {
                sh "./scripts/build-archive"
            }
            post {
                always {
                    archiveArtifacts artifacts: 'noteahead*.tar.gz', fingerprint: true
                }
            }
        }
    }
}
