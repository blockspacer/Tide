/*********************************************************************/
/* Copyright (c) 2013-2018, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#include "SessionLoader.h"

#include "ContentValidator.h"
#include "SessionLoaderLegacyXml.h"
#include "control/DisplayGroupController.h"
#include "scene/ContentFactory.h"
#include "scene/Scene.h"
#include "serialization/utils.h"
#include "utils/log.h"

namespace
{
void _adjust(DisplayGroup& group, const DisplayGroup& referenceGroup)
{
    // Reshape the new DisplayGroup only if it doesn't fit (legacy behaviour).
    // If the saved group was smaller, resize it but don't modify its windows.
    if (!referenceGroup.getCoordinates().contains(group.getCoordinates()))
        DisplayGroupController{group}.reshape(referenceGroup.size());
    else
    {
        group.setWidth(referenceGroup.width());
        group.setHeight(referenceGroup.height());
    }
}

void _adjust(Scene& scene, const Scene& referenceScene)
{
    const auto max =
        std::min(scene.getSurfaceCount(), referenceScene.getSurfaceCount());

    for (auto i = 0u; i < max; ++i)
        _adjust(scene.getGroup(i), referenceScene.getGroup(i));
}

void _adjust(Session& session, const Scene& referenceScene)
{
    auto scene = session.getScene();
    auto& group = scene->getGroup(0);

    DisplayGroupController controller{group};
    controller.updateFocusedWindowsCoordinates();

    if (session.getInfo().version < FIRST_PIXEL_COORDINATES_FILE_VERSION)
        controller.denormalize(referenceScene.getGroup(0).size());
    else if (session.getInfo().version == FIRST_PIXEL_COORDINATES_FILE_VERSION)
    {
        // Approximation; only applies to FIRST_PIXEL_COORDINATES_FILE_VERSION
        // which did not serialize the size of the DisplayGroup
        assert(group.getCoordinates().isEmpty());
        group.setCoordinates(controller.estimateSurface());
    }

    _adjust(*scene, referenceScene);
}

Session _load(const QString& filepath, const Scene& referenceScene)
{
    Session session;

    // For backward compatibility, try to load the file as a legacy xml first
    try
    {
        session = SessionLoaderLegacyXml().load(filepath);
    }
    catch (const std::runtime_error& e)
    {
        print_log(LOG_DEBUG, LOG_GENERAL,
                  "Not a valid legacy session file: '%s' - '%s'",
                  filepath.toLocal8Bit().constData(), e.what());
    }

    if (!serialization::fromXmlFile(session, filepath.toStdString()))
        return session;
    session.setFilepath(filepath);

    ContentValidator().validateContents(*session.getScene());
    _adjust(session, referenceScene);
    return session;
}
}

SessionLoader::SessionLoader(ScenePtr scene)
    : _scene{std::move(scene)}
{
}

QFuture<Session> SessionLoader::load(const QString& filepath) const
{
    print_log(LOG_INFO, LOG_CONTENT, "Restoring session: '%s'",
              filepath.toStdString().c_str());

    return QtConcurrent::run([ referenceScene = _scene, filepath ]() {
        auto session = _load(filepath, *referenceScene);
        if (auto scene = session.getScene())
            scene->moveToThread(QCoreApplication::instance()->thread());
        return session;
    });
}